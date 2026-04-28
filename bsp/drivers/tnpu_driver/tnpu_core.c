/*
 * Ingenic TNPU driver
 *
 * Copyright (c) 2023 LiuTianyang
 *
 * This file is released under the GPLv2
 */

#include "tnpu_drv.h"
#include "tnpu_register.h"

#include "tnpu_boot_code_fence.h"

#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/file.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <asm/r4kcache.h>
#include <asm/mipsregs.h>

#ifdef CONFIG_TNPU_CORE_DEBUG
#define PRINT_TNPU_DEBUG(fmt, ...) \
    printk(KERN_DEBUG "<TNPU> " fmt, ##__VA_ARGS__)
#else
#define PRINT_TNPU_DEBUG(fmt, ...)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
  extern void (*r4k_blast_dcache)(void);
#endif // LINUX_VERSION

#define L1_L2_CACHE_CONNECTION    (1 << 29)

#define TNPU_R_RISCV_32           1
#define TNPU_R_RISCV_RELATIVE     3
#define TNPU_R_RISCV_JUMP_SLOT    5

struct tnpu_libref {
    struct list_head               node;
    unsigned int                   paddr;
    pid_t                          pid;
};

struct tnpu_sharelib {
    struct list_head               node;
    char                           name[32];
    int                            ref_cnt;
    unsigned int                   size;
    void                          *kva;
    unsigned int                   paddr;
    Elf32_Dyn                     *dyn;
    char                          *strtab;
    Elf32_Sym                     *symtab;
    unsigned int                   sym_num;
    unsigned int                   need_lib[4];
    unsigned int                   need_lib_num;
};

struct tnpu_core_dev {
    struct device                  *dev;
    void __iomem                   *ccu_regs;
    void __iomem                   *dram_regs;
    int                             tnpu_irq;
    wait_queue_head_t               wait_queue;
    spinlock_t                      task_spin;
    struct mutex                    load_mutex;
    struct list_head                libref_list;
    struct list_head                sharelib_list;
    int (*libread)(struct file *elf,
            unsigned int uvaddr, loff_t off,
            void *dst_vaddr, size_t size);
    struct clk                     *div_tnpu;
    struct clk                     *gate_tnpu;
    int                             is_open;
#ifdef CONFIG_TNPU_DUTY_RATIO_DEBUG
    ktime_t                         basic_timer;
    ktime_t                         start_timer;
    unsigned long                   run_time;
    int                             duty_status;
#endif // CONFIG_TNPU_DUTY_RATIO_DEBUG
};
static struct tnpu_core_dev tnpu;

static void __iomem *tnpu_ioremap_regs(struct platform_device *pdev, int index)
{
    struct resource *res;
    void __iomem *map;

    res = platform_get_resource(pdev, IORESOURCE_MEM, index);
    map = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(map)) {
        PRINT_TNPU_ERROR("Failed to map registers: %ld\n", PTR_ERR(map));
        return map;
    }

    return map;
}

static irqreturn_t tnpu_irq_handler(int irq, void *data)
{
    unsigned int done_id, chn;
    int tohost = *TNPU_CCU_TOHOST;

#ifdef CONFIG_TNPU_SCHEDUL_DEBUG
    int done_chn;
    volatile unsigned long long done_time1;
    volatile unsigned long long done_time2;
    volatile unsigned long long done_time3 =
        ((unsigned long long)(*TNPU_CCU_TIME_H) << 32) | *TNPU_CCU_TIME_L;
    volatile unsigned long long done_time4;
#endif // CONFIG_TNPU_SCHEDUL_DEBUG

    *TNPU_CCU_TOHOST = 0;

    PRINT_TNPU_DEBUG("Received IRQ=%d.\n", tohost);
    if (tohost != 1) {
        int cmd = tohost & 0xf;
        int chn = (tohost >> 4) & 0xf;
        switch (cmd) {
        case TNPU_TOHOST_RET_EXL:
            PRINT_TNPU_ERROR("job=%d,pc=0x%x,abnormal exit!\n",
                    *TNPU_DRAM_TASK_ID(chn), *TNPU_DRAM_TASK_PC(chn));
            break;
        case TNPU_TOHOST_RET_TNNA:
            PRINT_TNPU_ERROR("job=%d,pc=0x%x,terminated abnormally in TNNA!\n",
                    *TNPU_DRAM_TASK_ID(chn), *TNPU_DRAM_TASK_PC(chn));
            break;
        case TNPU_TOHOST_RET_CVA:
            PRINT_TNPU_ERROR("job=%d,pc=0x%x,terminated abnormally in CVA!\n",
                    *TNPU_DRAM_TASK_ID(chn), *TNPU_DRAM_TASK_PC(chn));
            break;
        case TNPU_TOHOST_RET_STACK:
            PRINT_TNPU_ERROR("job=%d,pc=0x%x,stack overflow!\n",
                    *TNPU_DRAM_TASK_ID(chn), *TNPU_DRAM_TASK_PC(chn));
            break;
        }
        PRINT_TNPU_ERROR("Please check algorithm,mepc=0x%x,mtval=0x%x,type=%d,mstatus=0x%x\n",
                *TNPU_DRAM_TASK_ARG0(chn), *TNPU_DRAM_TASK_ARG1(chn),
                *TNPU_DRAM_TASK_ARG2(chn), *TNPU_DRAM_TASK_ARG3(chn));
    }

    if (*TNPU_DRAM_SHM_END != TNPU_STACK_MAJIC) {
        PRINT_TNPU_ERROR("tnpu stack overflow, id=%d,pc=0x%x\n",
                *TNPU_DRAM_TASK_ID(chn), *TNPU_DRAM_TASK_PC(chn));
        BUG();
    }

    done_id = *TNPU_DRAM_SHM_TASK_DONE_ID;
    chn = done_id % TNPU_MAX_TASK;

    if (*TNPU_DRAM_TASK_PID(chn) != 0) {
        PRINT_TNPU_DEBUG("IRQ run_next,id=0x%x\n", done_id);
        *TNPU_CCU_FROMHOST = 1;
    } else {
        PRINT_TNPU_DEBUG("IRQ stop\n");
        *TNPU_DRAM_SHM_RUN_STATUS = 0;
    }

#ifdef CONFIG_TNPU_SCHEDUL_DEBUG
    done_time4 = ((unsigned long long)(*TNPU_CCU_TIME_H) << 32) | *TNPU_CCU_TIME_L;
    wake_up_all(&tnpu.wait_queue);
    done_chn = (*TNPU_DRAM_SHM_TASK_DONE_ID - 1) % 8;
    done_time1 = ((unsigned long long)(*TNPU_DRAM_TASK_START_TIME_H(done_chn)) << 32)|
            *TNPU_DRAM_TASK_START_TIME_L(done_chn);
    done_time2 = ((unsigned long long)(*TNPU_DRAM_TASK_STOP_TIME_H(done_chn)) << 32)|
            *TNPU_DRAM_TASK_STOP_TIME_L(done_chn);
    PRINT_TNPU_INFO("IRQ:chn=%d, timer1=%lld, timer2=%lld, timer3=%lld\n",
            done_chn, done_time1, done_time2, done_time3);
#else
    wake_up_all(&tnpu.wait_queue);
#endif // CONFIG_TNPU_SCHEDUL_DEBUG

#ifdef CONFIG_TNPU_DUTY_RATIO_DEBUG
    if (*TNPU_DRAM_SHM_RUN_STATUS == 0 || (tnpu.duty_status == 3)) {
        tnpu.duty_status = 2;
        tnpu.run_time += ktime_to_us(ktime_sub(ktime_get(), tnpu.start_timer));
    }
#endif // CONFIG_TNPU_DUTY_RATIO_DEBUG

    return IRQ_HANDLED;
}

void tnpu_timer_open(void)
{
    *TNPU_CCU_CCSR |= 1 << 5;
    *TNPU_CCU_TIME_L = 0;
    *TNPU_CCU_TIME_H = 0;
}

void tnpu_timer_close(void)
{
    *TNPU_CCU_CCSR &= ~(1 << 5);
}

static inline void tnpu_hardware_stop(void)
{
    *TNPU_CCU_SOFT_RESET = CCU_SOFT_RESET_CMD_SET;
}

static inline void tnpu_hardware_load(void)
{
    volatile unsigned int *chip_info = (volatile unsigned int *)0xb300002c;
    unsigned int chip_id = (*chip_info >> 12) & 0xff;

    //if (chip_id == 0x33) {
    //    memcpy((void *)TNPU_DRAM_BOOT_START,
    //            (void *)tnpu_boot_code_fence, sizeof(tnpu_boot_code_fence));
    //} else if (chip_id == 0x32) {
    //    memcpy((void *)TNPU_DRAM_BOOT_START,
    //            (void *)tnpu_boot_code_nofence, sizeof(tnpu_boot_code_nofence));
    //} else {
    //    PRINT_TNPU_ERROR("Failed to get chip id, 0x%x\n", *chip_info);
    //}
    memcpy((void *)TNPU_DRAM_BOOT_START,
            (void *)tnpu_boot_code_fence, sizeof(tnpu_boot_code_fence));
}

static int tnpu_hardware_reset(void)
{
    volatile unsigned int ccsr;
    int cnt = 0;

    *TNPU_CCU_SOFT_RESET = CCU_SOFT_RESET_CMD_SET;
    ndelay(10);
    tnpu_hardware_load();

    *TNPU_DRAM_SHM_TASK_DONE_ID = 0x0;
    *TNPU_DRAM_SHM_RUN_STATUS = 0;
    *TNPU_DRAM_SHM_TASK_MAX_ID = 0x0;

    *TNPU_DRAM_SHM_END = TNPU_STACK_MAJIC;

    *TNPU_CCU_SOFT_RESET = CCU_SOFT_RESET_CMD_RELEASE;

    do {
        ndelay(80);
        ccsr = *TNPU_CCU_CCSR;
        cnt++;
        if (cnt > 0x3ff)
            return -ETIME;
    } while(!(ccsr & TNPU_CCU_CCSR_SLEEP_MASK));

    return 0;
}

int tnpu_clk_open(void)
{
    int ret;

    mutex_lock(&tnpu.load_mutex);
    if (tnpu.is_open) {
        mutex_unlock(&tnpu.load_mutex);
        return 0;
    }

    tnpu.is_open = 1;

    ret = clk_prepare_enable(tnpu.div_tnpu);
    if (ret) {
        PRINT_TNPU_ERROR("Failed to open div_tnpu, %d\n", ret);
        return ret;
    }
    ret = clk_prepare_enable(tnpu.gate_tnpu);
    if (ret) {
        PRINT_TNPU_ERROR("Failed to open gate_tnpu, %d\n", ret);
        return ret;
    }
    mutex_unlock(&tnpu.load_mutex);

    return tnpu_hardware_reset();
}

void tnpu_clk_close(void)
{
    mutex_lock(&tnpu.load_mutex);
    if (tnpu.is_open) {
        tnpu_hardware_stop();

        clk_disable(tnpu.div_tnpu);
        clk_disable(tnpu.gate_tnpu);
        tnpu.is_open = 0;
    }
    mutex_unlock(&tnpu.load_mutex);
}

int tnpu_core_init(struct platform_device *pdev,
            const char *parent_clk_name,
            unsigned long tnpu_clk_rate)
{
    int ret;
    unsigned long rate;
    struct clk *parent_clk;
#ifdef CONFIG_KERNEL_4_4_94
    struct clk *mux_tnpu;
#endif

    tnpu.ccu_regs = tnpu_ioremap_regs(pdev, 0);
    if (IS_ERR(tnpu.ccu_regs))
        return PTR_ERR(tnpu.ccu_regs);

    tnpu.dram_regs = tnpu_ioremap_regs(pdev, 1);
    if (IS_ERR(tnpu.dram_regs))
        return PTR_ERR(tnpu.dram_regs);

    tnpu.tnpu_irq = platform_get_irq(pdev, 0);
    if (tnpu.tnpu_irq < 0)
        return tnpu.tnpu_irq;

    enable_irq(tnpu.tnpu_irq);

    ret = devm_request_irq(&pdev->dev, tnpu.tnpu_irq,
            tnpu_irq_handler, IRQF_SHARED, "tnpu-irq", &tnpu);
    if (ret) {
        PRINT_TNPU_ERROR("request_irq fail\n");
        return ret;
    }

    if (read_c0_ecc() & L1_L2_CACHE_CONNECTION) {
        // L1-L2 direct connection, just flush L1 dcache
        tnpu.dev = NULL;
    } else {
        // L1-L2 is not directly connected, need to flush L2 cache
        tnpu.dev = &pdev->dev;
    }
    init_waitqueue_head(&tnpu.wait_queue);
    spin_lock_init(&tnpu.task_spin);
    mutex_init(&tnpu.load_mutex);
    INIT_LIST_HEAD(&tnpu.libref_list);
    INIT_LIST_HEAD(&tnpu.sharelib_list);

    parent_clk = clk_get(NULL, parent_clk_name);
    if (IS_ERR(parent_clk)) {
        PRINT_TNPU_ERROR("Failed to get parent clock\n");
        return IS_ERR(parent_clk);
    }
    tnpu.div_tnpu = clk_get(NULL, "div_tnpu");
    if (IS_ERR(tnpu.div_tnpu)) {
        PRINT_TNPU_ERROR("Failed to get div_tnpu clock\n");
        return IS_ERR(tnpu.div_tnpu);
    }
    tnpu.gate_tnpu = clk_get(NULL, "gate_tnpu");
    if (IS_ERR(tnpu.gate_tnpu)) {
        PRINT_TNPU_ERROR("Failed to get gate_tnpu clock\n");
        return IS_ERR(tnpu.gate_tnpu);
    }

#ifdef CONFIG_KERNEL_4_4_94
    mux_tnpu = clk_get(NULL, "mux_tnpu");
    if (IS_ERR(mux_tnpu)) {
        PRINT_TNPU_ERROR("Failed to get mux clock\n");
        return IS_ERR(mux_tnpu);
    }
    clk_set_parent(mux_tnpu, parent_clk);
#else
    clk_set_parent(tnpu.div_tnpu, parent_clk);
#endif
    ret = clk_prepare_enable(tnpu.div_tnpu);
    if (ret) {
        PRINT_TNPU_ERROR("Failed to open div_tnpu, %d\n", ret);
        return ret;
    }
    ret = clk_prepare_enable(tnpu.gate_tnpu);
    if (ret) {
        PRINT_TNPU_ERROR("Failed to open gate_tnpu, %d\n", ret);
        return ret;
    }
    clk_set_rate(tnpu.div_tnpu, tnpu_clk_rate);
    rate = clk_get_rate(tnpu.div_tnpu) / 1000000;
    PRINT_TNPU_INFO("tnna_clk=%ldMHz, v0_clk=%ldMHz, default=%ldMHz\n",
            rate << 1, rate, tnpu_clk_rate / 1000000);

    tnpu_hardware_load();
    tnpu.is_open = 1;
    tnpu_clk_close();

    return 0;
    //return tnpu_hardware_init();
}


static int tnpu_wait(unsigned int id, unsigned long timeout_us)
{
    wait_queue_head_t *wq = &tnpu.wait_queue;
    unsigned long timeout_jiffies;
    unsigned long timeout_expire;
    unsigned long start_jiffies = jiffies;
    DEFINE_WAIT(wait);

    if (timeout_us > 0) {
        timeout_jiffies = usecs_to_jiffies(timeout_us);
        if (timeout_jiffies < 2)
            timeout_jiffies = 2;
        timeout_expire = start_jiffies + timeout_jiffies;
    }

    if (*TNPU_DRAM_SHM_TASK_DONE_ID > id) {
        PRINT_TNPU_DEBUG("Finish wait, task_id=0x%x, done_id=%d.\n",
                id, *TNPU_DRAM_SHM_TASK_DONE_ID);
        return 0;
    }
    // Dealing with overflow situations
    if (id > 0xfffffff8 && *TNPU_DRAM_SHM_TASK_DONE_ID < 8) {
        PRINT_TNPU_DEBUG("Finish overflow wait, task_id=%d, done_id=%d.\n",
                id, *TNPU_DRAM_SHM_TASK_DONE_ID);
        return 0;
    }

    for (;;) {
        prepare_to_wait(wq, &wait, TASK_INTERRUPTIBLE);

        if (*TNPU_DRAM_SHM_TASK_DONE_ID > id)
            break;

        if (id > 0xfffffff8 && *TNPU_DRAM_SHM_TASK_DONE_ID < 8) {
            PRINT_TNPU_DEBUG("overflow wait, task_id=%d.\n", id);
            break;
        }

        if (signal_pending(current)) {
            finish_wait(wq, &wait);
            PRINT_TNPU_ERROR("The waiting was terminated by unknown signal.\n");
            msleep(20);
            return -ERESTARTSYS;
        }

        if (timeout_us == 0) {
            schedule();
        } else {
            if (time_after_eq(jiffies, timeout_expire)) {
                finish_wait(wq, &wait);
                PRINT_TNPU_ERROR("Timeout occurred while waiting.\n");
                return -ETIME;
            }
            schedule_timeout(timeout_jiffies);
        }
    }

    finish_wait(wq, &wait);

    PRINT_TNPU_DEBUG("Finish wait, task_id=0x%x, done_id=%d.\n",
            id, *TNPU_DRAM_SHM_TASK_DONE_ID);

    return 0;
}

int ioctl_tnpu_job_wait(unsigned long arg)
{
    int ret;
    tnpu_ioctl_wait_t user_wait;

    ret = copy_from_user(&user_wait,
            (tnpu_ioctl_wait_t __user *)arg, sizeof(user_wait));
    if (ret) {
        PRINT_TNPU_ERROR("Failed to copy op from user\n");
        return ret;
    }

    if (*TNPU_DRAM_SHM_TASK_MAX_ID == 0) {
        // Bug, Exclude Previous Versions
        if (user_wait.seqno > 0x1000) {
            PRINT_TNPU_ERROR("The waiting seqno is 0x%x, max_id=0x%x, done_id=0x%x\n",
                user_wait.seqno, *TNPU_DRAM_SHM_TASK_MAX_ID, *TNPU_DRAM_SHM_TASK_DONE_ID);
            PRINT_TNPU_ERROR("Abnormal operation of library occurred!\n");
            PRINT_TNPU_ERROR("Please update the libaie.so.\n");
            return -EFAULT;
        }
    }

    return tnpu_wait(user_wait.seqno, user_wait.timeout_us);
}

static int tnpu_read_from_file(struct file *elf, unsigned int uvaddr,
        loff_t off, void *dst_vaddr, size_t size)
{
    return kernel_read(elf, off, dst_vaddr, size);
}

static int tnpu_read_from_mem(struct file *elf, unsigned int uvaddr,
        loff_t off, void *dst_vaddr, size_t size)
{
    return copy_from_user(dst_vaddr, (void *)((char *)uvaddr + off), size);
}

static void tnpu_free_lib(unsigned int paddr)
{
    struct tnpu_sharelib *lib, *tmp_lib;

    list_for_each_entry_safe_reverse(lib, tmp_lib, &tnpu.sharelib_list, node) {
        if (lib->paddr == paddr) {
            lib->ref_cnt--;
            if (lib->ref_cnt == 0) {
                int i;
                list_del(&lib->node);
                for (i = 0; i < lib->need_lib_num; i++) {
                    tnpu_free_lib(lib->need_lib[i]);
                }
                tnpu_cma_free(lib->size, lib->kva, lib->paddr);
                kfree(lib);
                PRINT_TNPU_DEBUG("%s has been uninstalled.\n", lib->name);
                break;
            }
        }
    }
}

static unsigned int tnpu_find_func_offset(const char *rela_name)
{
    struct tnpu_sharelib *need_lib;
    unsigned int need_off;
    char *need_name;
    int j;

    list_for_each_entry_reverse(need_lib, &tnpu.sharelib_list, node) {
        for (j = 1; j < need_lib->sym_num; j++) {
            need_off = need_lib->symtab[j].st_value;
            if (!need_off)
                continue;
            need_name = need_lib->strtab + need_lib->symtab[j].st_name;
            if (!strcmp(need_name, rela_name)) {
                PRINT_TNPU_DEBUG("  LINK FUNC:%s of %s,addr:0x%x off:0x%x",
                        need_name, need_lib->name, need_lib->paddr, need_off);
                return need_lib->paddr + need_off;
            }
        }
    }

    return 0;
}

static unsigned int tnpu_get_lib(const char *name, unsigned int vaddr)
{
    struct tnpu_sharelib *lib;
    int ret, i, rela_off = 0;
    struct file *elf;
    struct elfhdr ehdr;
    struct elf_phdr phdr, load1, load2;
    Elf32_Dyn *dyn;

    list_for_each_entry(lib, &tnpu.sharelib_list, node) {
        if (!strcmp(name, lib->name)) {
            PRINT_TNPU_DEBUG("%s lib already loaded.\n", name);
            lib->ref_cnt++;
            return lib->paddr;
        }
    }

    if (vaddr == 0) {
        elf = open_exec(name);
        if (IS_ERR(elf)) {
            PRINT_TNPU_ERROR("Can't open %s library.\n", name);
            return 0;
        }
        tnpu.libread = tnpu_read_from_file;
    } else {
        elf = NULL;
        tnpu.libread = tnpu_read_from_mem;
    }

    ret = tnpu.libread(elf, vaddr, 0, (char *)&ehdr, sizeof(struct elfhdr));
    if (ret < 0) {
        PRINT_TNPU_ERROR("Failed to read ELF file header\n");
        if (vaddr == 0) fput(elf);
        return 0;
    }


    load1.p_memsz = 0;
    load2.p_memsz = 0;
    for (i = 0; i < ehdr.e_phnum; i++) {
        ret = tnpu.libread(elf, vaddr, ehdr.e_phoff + (i * sizeof(struct elf_phdr)),
                    (char *)(&phdr), sizeof(struct elf_phdr));
        if (ret < 0) {
            PRINT_TNPU_ERROR("Failed to read phdr\n");
            if (vaddr == 0) fput(elf);
            return 0;
        }
        if (phdr.p_type == PT_LOAD) {
            if (load1.p_memsz == 0) {
                load1.p_type = PT_LOAD;
                load1.p_offset = phdr.p_offset;
                load1.p_filesz = phdr.p_filesz;
                load1.p_memsz = phdr.p_memsz;

            } else {
                load2.p_type = PT_LOAD;
                load2.p_offset = phdr.p_offset;
                load2.p_filesz = phdr.p_filesz;
                load2.p_memsz = phdr.p_memsz;
            }
        }
        if (phdr.p_type == PT_DYNAMIC)
            break;
    }
    if (phdr.p_type != PT_DYNAMIC) {
        PRINT_TNPU_ERROR("Failed to find phdr\n");
        if (vaddr == 0) fput(elf);
        return 0;
    }

    lib = kmalloc(sizeof(*lib), GFP_KERNEL);
    strcpy(lib->name, name);
    lib->ref_cnt = 1;
    lib->size = round_up(load1.p_memsz + load2.p_memsz, PAGE_SIZE);
    lib->kva = tnpu_cma_alloc(lib->size, &lib->paddr);
    if (!lib->kva) {
        PRINT_TNPU_ERROR("Failed to alloc cma for lib.\n");
        kfree(lib);
        if (vaddr == 0) fput(elf);
        return 0;
    }
    lib->dyn = (Elf32_Dyn *)((char *)lib->kva + phdr.p_offset);
    dyn = lib->dyn;

    // LOAD1
    ret = tnpu.libread(elf, vaddr, load1.p_offset,
            (char *)lib->kva, load1.p_filesz);
    if (ret < 0) {
        tnpu_cma_free(lib->size, lib->kva, lib->paddr);
        kfree(lib);
        PRINT_TNPU_ERROR("Failed to read load1\n");
        if (vaddr == 0) fput(elf);
        return 0;
    }
    // LOAD2
    if (load2.p_memsz > 0) {
        ret = tnpu.libread(elf, vaddr, load2.p_offset,
                (char *)lib->kva + load1.p_memsz, load2.p_filesz);
        if (ret < 0) {
            tnpu_cma_free(lib->size, lib->kva, lib->paddr);
            kfree(lib);
            PRINT_TNPU_ERROR("Failed to read load2\n");
            if (vaddr == 0) fput(elf);
            return 0;
        }
    }
    if (vaddr == 0) fput(elf);

    // LINK START
    dyn = dyn + 2;
    while (dyn->d_tag != DT_RELASZ) {
        switch (dyn->d_tag) {
            case DT_STRTAB:
                lib->strtab = (char *)lib->kva + dyn->d_un.d_val;
                break;
            case DT_SYMTAB:
                lib->symtab = (Elf_Sym *)((char *)lib->kva + dyn->d_un.d_val);
                break;
            case DT_RELA:
                rela_off = dyn->d_un.d_val;
                break;
        }
        dyn++;
    }
    lib->sym_num = (unsigned int)(lib->strtab - (char *)lib->symtab) / 16;

    lib->need_lib_num = 0;
    if (rela_off) {
        Elf32_Rela *rela = (Elf_Rela *)((char *)lib->kva + rela_off);
        unsigned int relasz, relaend, rela_num;

        dyn = lib->dyn;
        while (dyn->d_tag != DT_NULL) {
            switch (dyn->d_tag) {
                case DT_NEEDED:
                    PRINT_TNPU_DEBUG("%s needed other lib %s.\n",
                            lib->name, lib->strtab + dyn->d_un.d_val);
                    lib->need_lib[lib->need_lib_num] =
                            tnpu_get_lib(lib->strtab + dyn->d_un.d_val, 0);
                    if (lib->need_lib[lib->need_lib_num] == 0) {
                        kfree(lib);
                        if (vaddr == 0) fput(elf);
                        return 0;
                    }
                    lib->need_lib_num++;
                    break;
                case DT_RELASZ:
                    relasz = dyn->d_un.d_val;
                    break;
                case DT_RELAENT:
                    relaend = dyn->d_un.d_val;
                    break;
            }
            dyn++;
        }
        rela_num = relasz / relaend;

        for (i = 0; i < rela_num; i++) {
            unsigned int *rela_pos = (unsigned int *)((char *)lib->kva + rela[i].r_offset);
            int rela_type = ELF_R_TYPE(rela[i].r_info);
            int rela_idx = ELF_R_SYM(rela[i].r_info);
            char *rela_name = lib->strtab + lib->symtab[rela_idx].st_name;
            PRINT_TNPU_DEBUG("  Link pos=%p,type=%d,idx=%d,name=%s\n",
                    rela_pos, rela_type, rela_idx, rela_name);
            if (rela_type == TNPU_R_RISCV_JUMP_SLOT ||
                    rela_type == TNPU_R_RISCV_32) {
                int link_off = lib->symtab[rela_idx].st_value;
                if (!link_off) {
                    *rela_pos = tnpu_find_func_offset(rela_name);
                    if (*rela_pos == 0) {
                        PRINT_TNPU_ERROR("Function missing,%s,func=%s\n",
                                lib->name, rela_name);
                        kfree(lib);
                        if (vaddr == 0) fput(elf);
                        return 0;
                    }
                } else {
                    *rela_pos = lib->paddr + link_off;
                }
            } else if (rela_type == TNPU_R_RISCV_RELATIVE) {
                *rela_pos = lib->paddr + rela[i].r_addend;
            } else {
                PRINT_TNPU_ERROR("%s link err, rela_type=%d not support\n", lib->name, rela_type);
                kfree(lib);
                if (vaddr == 0) fput(elf);
                return 0;
            }
        }
    }
    // LINK END
    
    list_add_tail(&lib->node, &tnpu.sharelib_list);

    PRINT_TNPU_DEBUG("Complete the loading of %s,addr=0x%x,size=%d\n",
            lib->name, lib->paddr, lib->size);
    return lib->paddr;
}

int ioctl_tnpu_sharelib_load(unsigned long arg)
{
    tnpu_ioctl_sharelib_t load_lib;
    struct tnpu_libref *libref;

    if (copy_from_user(&load_lib, (void __user *)arg, sizeof(load_lib))) {
        PRINT_TNPU_ERROR("Failed to copy from user\n");
        return -EFAULT;
    }

    libref = kmalloc(sizeof(*libref), GFP_KERNEL);
    if (!libref) {
        return -ENOMEM;
    }

    mutex_lock(&tnpu.load_mutex);

    libref->paddr = tnpu_get_lib(load_lib.name, load_lib.uvaddr);
    if (libref->paddr == 0) {
        mutex_unlock(&tnpu.load_mutex);
        kfree(libref);
        return -EFAULT;
    }
    libref->pid = task_tgid_nr(current);

    list_add_tail(&libref->node, &tnpu.libref_list);

    mutex_unlock(&tnpu.load_mutex);

    load_lib.paddr = libref->paddr;

    return copy_to_user((void __user *)arg, &load_lib, sizeof(load_lib));
}

int ioctl_tnpu_sharelib_unload(unsigned long arg)
{
    unsigned int lib_paddr;
    unsigned int pid = task_tgid_nr(current);
    struct tnpu_libref *libref;

    if (copy_from_user(&lib_paddr, (void __user *)arg, sizeof(unsigned int))) {
        PRINT_TNPU_ERROR("Failed to copy from user\n");
        return -EFAULT;
    }

    mutex_lock(&tnpu.load_mutex);
    list_for_each_entry_reverse(libref, &tnpu.libref_list, node) {
        if (libref->paddr == lib_paddr && libref->pid == pid) {
            tnpu_free_lib(lib_paddr);
            list_del(&libref->node);
            kfree(libref);
            break;
        }
    }
    mutex_unlock(&tnpu.load_mutex);

    return 0;
}

int ioctl_tnpu_job_submit(unsigned long arg)
{
    tnpu_ioctl_submit_t submit;

    unsigned long irqflags;
    unsigned int curr_id, done_id, chn;
    unsigned int curr_pid = task_tgid_nr(current);
    PRINT_TNPU_DEBUG("Task submit,pid=%d\n", curr_pid);

#ifdef CONFIG_TNPU_SCHEDUL_DEBUG
    volatile unsigned int submit1_time_l, submit1_time_h;
    volatile unsigned int submit2_time_l, submit2_time_h;
    submit1_time_l = *TNPU_CCU_TIME_L;
    submit1_time_h = *TNPU_CCU_TIME_H;
#endif // CONFIG_TNPU_SCHEDUL_DEBUG

    if (copy_from_user(&submit, (void __user *)arg, sizeof(submit))) {
        PRINT_TNPU_ERROR("Failed to copy from user.\n");
        return -EFAULT;
    }

    if (submit.ctl & TNPU_JOB_DELAY) {
        int i;
        for (i = 0; i < TNPU_JOB_DELAY_DENSITY; i++) {
            done_id = *TNPU_DRAM_SHM_TASK_DONE_ID;
            if (done_id == *TNPU_DRAM_SHM_TASK_MAX_ID)
                break;
            PRINT_TNPU_DEBUG("Task delay,done_id=0x%x,pid=%d\n", done_id, curr_pid);
            tnpu_wait(done_id, 0);
        }
    }

    spin_lock_irqsave(&tnpu.task_spin, irqflags);

    curr_id = *TNPU_DRAM_SHM_TASK_MAX_ID;
    *TNPU_DRAM_SHM_TASK_MAX_ID = curr_id + 1;
    chn = curr_id % TNPU_MAX_TASK;

    done_id = *TNPU_DRAM_SHM_TASK_DONE_ID;
    if (TNPU_MAX_TASK < (curr_id - done_id)) {
        if (65535 > (curr_id - done_id)) {
            // Avoid integer overflow
            spin_unlock_irqrestore(&tnpu.task_spin, irqflags);
            PRINT_TNPU_DEBUG("Task wait,curr_id=0x%x,done_id=%d\n", curr_id, done_id);
            tnpu_wait(curr_id - TNPU_MAX_TASK + 2, 0);
            spin_lock_irqsave(&tnpu.task_spin, irqflags);
            done_id = *TNPU_DRAM_SHM_TASK_DONE_ID;
        }
    }

#ifdef CONFIG_TNPU_DUTY_RATIO_DEBUG
    if (tnpu.duty_status == 2) {
        tnpu.start_timer = ktime_get();
        tnpu.duty_status = 3;
    }
#endif // CONFIG_TNPU_DUTY_RATIO_DEBUG

    *TNPU_DRAM_TASK_ID(chn) = curr_id;
    *TNPU_DRAM_TASK_PID(chn) = curr_pid;
    *TNPU_DRAM_TASK_PC(chn) = submit.pc;
    *TNPU_DRAM_TASK_ARG0(chn) = submit.arg[0];
    *TNPU_DRAM_TASK_ARG1(chn) = submit.arg[1];
    *TNPU_DRAM_TASK_ARG2(chn) = submit.arg[2];
    *TNPU_DRAM_TASK_ARG3(chn) = submit.arg[3];
    *TNPU_DRAM_TASK_PID(*TNPU_DRAM_SHM_TASK_MAX_ID % TNPU_MAX_TASK) = 0;

    if (!(submit.ctl & TNPU_KEEP_HOST_CACHE)) {
        if (tnpu.dev == NULL) {
            r4k_blast_dcache();
        } else {
            dma_cache_sync(tnpu.dev, tnpu.dram_regs, 0x20000, DMA_BIDIRECTIONAL);
        }
    }
    

#ifdef CONFIG_TNPU_SCHEDUL_DEBUG
    if (*TNPU_DRAM_SHM_RUN_STATUS == 0) {
        *TNPU_DRAM_SHM_RUN_STATUS = 1;
        submit2_time_l = *TNPU_CCU_TIME_L;
        submit2_time_h = *TNPU_CCU_TIME_H;
        *TNPU_CCU_FROMHOST = 1;
        spin_unlock_irqrestore(&tnpu.task_spin, irqflags);
        PRINT_TNPU_INFO("Task run,curr_id=%d,done_id=%d,pc=0x%x,timer1=%lld,timer2=%lld\n",
                curr_id, done_id, *TNPU_DRAM_TASK_PC(chn),
                (unsigned long long)submit1_time_h << 32 | submit1_time_l,
                (unsigned long long)submit2_time_h << 32 | submit2_time_l);
    } else {
        spin_unlock_irqrestore(&tnpu.task_spin, irqflags);
        submit2_time_l = *TNPU_CCU_TIME_L;
        submit2_time_h = *TNPU_CCU_TIME_H;
        PRINT_TNPU_INFO("Task wait,curr_id=%d,done_id=%d,status=%d,timer1=%lld,timer2=%lld\n",
                curr_id, done_id, *TNPU_DRAM_SHM_RUN_STATUS,
                (unsigned long long)submit1_time_h << 32 | submit1_time_l,
                (unsigned long long)submit2_time_h << 32 | submit2_time_l);
    }
#else
    if (*TNPU_DRAM_SHM_RUN_STATUS == 0) {
        *TNPU_DRAM_SHM_RUN_STATUS = 1;
        PRINT_TNPU_DEBUG("Task run,curr_id=0x%x,pc=0x%x,pid=%d\n",
                        curr_id, submit.pc, curr_pid);
        *TNPU_CCU_FROMHOST = 1;
    }
    spin_unlock_irqrestore(&tnpu.task_spin, irqflags);
#endif // CONFIG_TNPU_SCHEDUL_DEBUG

    submit.id = curr_id;
    if (copy_to_user(&((tnpu_ioctl_submit_t __user *)arg)->id,
            &submit.id, sizeof(unsigned int))) {
        PRINT_TNPU_ERROR("Failed to copy from user.\n");
        return tnpu_wait(curr_id, 0);
    }

    if (submit.ctl & TNPU_JOB_BLOCK) {
        return tnpu_wait(curr_id, 0);
    }

    return 0;
}

void tnpu_duty_ratio_init(void)
{
#ifdef CONFIG_TNPU_DUTY_RATIO_DEBUG
    unsigned long irqflags;
    tnpu.basic_timer = ktime_get();
    tnpu.run_time = 0;
    spin_lock_irqsave(&tnpu.task_spin, irqflags);
    if (*TNPU_DRAM_SHM_RUN_STATUS == 0) {
        tnpu.start_timer = ktime_set(0, 0);
        tnpu.duty_status = 2;
    }
    else {
        tnpu.start_timer = tnpu.basic_timer;
        tnpu.duty_status = 3;
    }
    spin_unlock_irqrestore(&tnpu.task_spin, irqflags);
#endif // CONFIG_TNPU_DUTY_RATIO_DEBUG
}

void tnpu_duty_ratio_show(unsigned long *all, unsigned long *run)
{
#ifdef CONFIG_TNPU_DUTY_RATIO_DEBUG
    unsigned long irqflags;
    *all =  ktime_to_us(ktime_sub(ktime_get(), tnpu.basic_timer));

    tnpu.basic_timer = ktime_get();

    spin_lock_irqsave(&tnpu.task_spin, irqflags);
    if (*TNPU_DRAM_SHM_RUN_STATUS == 0) {
        *run = tnpu.run_time;
    } else {
        *run = tnpu.run_time +
                ktime_to_us(ktime_sub(tnpu.basic_timer, tnpu.start_timer));
        tnpu.start_timer = tnpu.basic_timer;
    }
    tnpu.run_time = 0;
    spin_unlock_irqrestore(&tnpu.task_spin, irqflags);
#else
    *run = 0;
#endif // CONFIG_TNPU_DUTY_RATIO_DEBUG
}

void tnpu_duty_ratio_stop(void)
{
#ifdef CONFIG_TNPU_DUTY_RATIO_DEBUG
    tnpu.duty_status = 0;
#endif // CONFIG_TNPU_DUTY_RATIO_DEBUG
}

void tnpu_kill_job(pid_t pid)
{
    unsigned long irqflags;
    unsigned int i, run_id, run_chn;
    struct tnpu_libref *libref, *tmp_libref;

    // job
    spin_lock_irqsave(&tnpu.task_spin, irqflags);
    run_id = *TNPU_DRAM_SHM_TASK_DONE_ID;
    run_chn = run_id % TNPU_MAX_TASK;
    for (i = 0; i < TNPU_MAX_TASK; i++) {
        if (*TNPU_DRAM_TASK_PID(i) != pid)
            continue;
        *TNPU_DRAM_TASK_PC(i) = 0;
        *TNPU_DRAM_TASK_PID(i) = 0;
        if (i == run_chn && *TNPU_DRAM_SHM_RUN_STATUS) {
            int cnt = 0;
            *TNPU_CCU_BECR |= 1;
            do {
                cnt++;
                ndelay(40);
                if (cnt > 0x3ff) {
                    PRINT_TNPU_ERROR("Can not kill job.\n");
                    PRINT_TNPU_ERROR("TNPU is no longer running, please reboot!\n");
                    return;
                }
            } while((*TNPU_CCU_BECR & 6) != 6);
            *TNPU_CCU_SOFT_RESET = CCU_SOFT_RESET_CMD_SET;
            *TNPU_CCU_BECR &= ~(1 << 0);
            *TNPU_CCU_TOHOST = 0;

            cnt = 0;
            ndelay(20);
            *TNPU_CCU_SOFT_RESET = CCU_SOFT_RESET_CMD_RELEASE;
            do {
                ndelay(40);
                cnt++;
                if (cnt > 0x3ff) {
                    PRINT_TNPU_ERROR("Can not reset tnpu.\n");
                    PRINT_TNPU_ERROR("TNPU is no longer running, please reboot!\n");
                    break;
                }
            }while(!(*TNPU_CCU_CCSR & TNPU_CCU_CCSR_SLEEP_MASK));
            run_id++;
            *TNPU_DRAM_SHM_TASK_DONE_ID = run_id;
            wake_up_all(&tnpu.wait_queue);
            if (run_id < *TNPU_DRAM_SHM_TASK_MAX_ID) {
                *TNPU_CCU_FROMHOST = 1;
            } else {
                *TNPU_DRAM_SHM_RUN_STATUS = 0;
            }
        }
    }
    spin_unlock_irqrestore(&tnpu.task_spin, irqflags);

    // lib
    mutex_lock(&tnpu.load_mutex);
    list_for_each_entry_safe_reverse(libref, tmp_libref, &tnpu.libref_list, node) {
        if (libref->pid == pid) {
            tnpu_free_lib(libref->paddr);
            list_del(&libref->node);
            kfree(libref);
        }
    }
    mutex_unlock(&tnpu.load_mutex);
}
