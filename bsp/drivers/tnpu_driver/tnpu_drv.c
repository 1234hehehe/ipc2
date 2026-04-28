/*
 * Ingenic TNPU driver
 *
 * Copyright (c) 2023 LiuTianyang
 *
 * This file is released under the GPLv2
 */

#include "tnpu_drv.h"
#include "tnpu_register.h"
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/atomic.h>

#ifdef CONFIG_TNPU_DEV_DEBUG
#define PRINT_TNPU_DEBUG(fmt, ...) \
    printk(KERN_DEBUG "<TNPU> " fmt, ##__VA_ARGS__)
#else
#define PRINT_TNPU_DEBUG(fmt, ...)
#endif

struct tnpu_drv_manger {
    atomic_t                        open_cnt;
    struct platform_device          pdev;
    struct platform_driver          pdrv;
    struct miscdevice               misc;
    struct file_operations          fops;
    struct resource                 resource[3];
#ifdef CONFIG_PM
    struct dev_pm_ops               pm_ops;
#endif
};
static struct tnpu_drv_manger tnpu;

static char *parent_clk_name = "apll";
module_param(parent_clk_name, charp, S_IRUGO);
MODULE_PARM_DESC(tnpu_clk_patent, "Chose tnpu parent clk.(apll, vpll, mpll)");

static unsigned long tnpu_clk_rate = 600000000;
module_param(tnpu_clk_rate, ulong, S_IRUGO);
MODULE_PARM_DESC(tnpu_clk_rate, "Set tnpu clk rate.(Hz).");

static unsigned int cma_pool_size = 0;
module_param(cma_pool_size, uint, S_IRUGO);
MODULE_PARM_DESC(cma_pool_size, "Size of the CMA pool to occupy (in bytes)");

static void tnpu_device_release(struct device *dev) {
}

static int tnpu_probe(struct platform_device *pdev)
{
    int ret;

    if (cma_pool_size & 4095) {
        cma_pool_size = (cma_pool_size + 4095) & ~4095;
    }
    ret = tnpu_bo_init(&pdev->dev, cma_pool_size);
    if (ret)
        return ret;

    ret = tnpu_core_init(pdev, parent_clk_name, tnpu_clk_rate);
    if (ret)
        return ret;

    ret = misc_register(&tnpu.misc);
    if (ret)
        return ret;

    atomic_set(&tnpu.open_cnt, 0);
    PRINT_TNPU_DEBUG("Successful probe.\n");
    return 0;
}

static int tnpu_remove(struct platform_device *pdev)
{
    tnpu_bo_pool_remove();
    misc_deregister(&tnpu.misc);
    PRINT_TNPU_DEBUG("tnpu remove.\n");
    return 0;
}

static int tnpu_open(struct inode *indo, struct file *file)
{
    if (atomic_inc_return(&tnpu.open_cnt) == 1) {
        tnpu_clk_open();
        tnpu_timer_open();
        PRINT_TNPU_DEBUG("tnpu clk turned on.\n");
    }
    return 0;
}

static int tnpu_release(struct inode *inode, struct file *file)
{
    pid_t pid = task_tgid_nr(current);
    tnpu_kill_job(pid);    
    tnpu_bo_release(pid);

    if (atomic_dec_and_test(&tnpu.open_cnt)) {
        tnpu_timer_close();
        tnpu_clk_close();
        tnpu_duty_ratio_stop();
        PRINT_TNPU_DEBUG("tnpu clk turned off.\n");
    }
    PRINT_TNPU_DEBUG("tnpu release, pid[%d].\n", pid);
    return 0;
}

static ssize_t tnpu_read(struct file *file,
        char __user * buffer, size_t count, loff_t * ppos)
{
    int ret = 0;
    tnpu_read_comamnd_t cmd;

    ret = copy_from_user(&cmd, buffer, sizeof(cmd));
    if (ret) {
        PRINT_TNPU_ERROR("tnpu drv read cmd fail\n");
        return count;
    }

    if (cmd == TNPU_READ_TIMER) {
    } else if (cmd == TNPU_READ_POOL_AVAIL) {
        unsigned int avail = tnpu_pool_avail();
        ret = copy_to_user(buffer, &avail, count);
    } else if (cmd == TNPU_READ_POOL_SIZE) {
        unsigned int size = tnpu_pool_size();
        ret = copy_to_user(buffer, &size, count);
    } else if (cmd == TNPU_READ_DUTY_INIT) {
        tnpu_duty_ratio_init();
    } else if (cmd == TNPU_READ_DUTY_STOP) {
        tnpu_duty_ratio_stop();
    } else if (cmd == TNPU_READ_DUTY_SHOW) {
        tnpu_read_ratio_t ratio;
        tnpu_duty_ratio_show(&ratio.all, &ratio.run);
        ret = copy_to_user(buffer, &ratio, count);
    } else if (cmd == TNPU_READ_VERSION) {
        unsigned int version = TNPU_DRIVER_VERSION;
        ret = copy_to_user(buffer, &version, count);
    } else {
        PRINT_TNPU_ERROR("Can not support cmd:%d\n", cmd);
    }

    if (ret) {
        PRINT_TNPU_ERROR("tnpu drv read fail\n");
    }

    return count;
}

static ssize_t tnpu_write(struct file *file,
        const char __user * buffer, size_t count, loff_t * ppos)
{
    int ret;
    char command[count];
    unsigned long pool_info;

    if (copy_from_user(command, buffer, count)) {
        return -EFAULT;
    }

    if (strncmp(command, "alloc", 5) == 0) {
        char *num_start = command + 5;
        while (*num_start == ' ' || *num_start == '\t') {
            num_start++;
	}
        ret = kstrtoul(num_start, 0, &pool_info);
        if (ret) {
            PRINT_TNPU_ERROR("Invalid size format: %s\n", command);
            return count;
        }
        tnpu_bo_pool_add(0, pool_info);
    } else if (strncmp(command, "pool_paddr", 10) == 0) {
        char *num_start = command + 10;
        while (*num_start == ' ' || *num_start == '\t') {
            num_start++;
	}
        ret = kstrtoul(num_start, 10, &pool_info);
        if (ret) {
            PRINT_TNPU_ERROR("Invalid size format: %s\n", command);
            return count;
        }
        tnpu_bo_pool_add(pool_info, tnpu_pool_quota());
    } else if (strncmp(command, "free", 4) == 0) {
        tnpu_bo_pool_remove();
    } else if (strncmp(command, "show", 4) == 0) {
        tnpu_show_allocations();
    }

    return count;
}

static int tnpu_mmap(struct file *filp, struct vm_area_struct *vma)
{
    return 0;
}

static long tnpu_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case IOCTL_TNPU_BO_APPEND:
        return ioctl_tnpu_bo_append(arg);
    case IOCTL_TNPU_BO_DESTROY:
        return ioctl_tnpu_bo_destroy(arg);
    case IOCTL_TNPU_SHARELIB_LOAD:
        return ioctl_tnpu_sharelib_load(arg);
    case IOCTL_TNPU_SHARELIB_UNLOAD:
        return ioctl_tnpu_sharelib_unload(arg);
    case IOCTL_TNPU_JOB_WAIT:
        return ioctl_tnpu_job_wait(arg);
    case IOCTL_TNPU_JOB_SUBMIT:
        return ioctl_tnpu_job_submit(arg);
    case IOCTL_TNPU_POOL_QUOTA:
        return ioctl_tnpu_pool_quota(arg);
    case IOCTL_TNPU_POOL_CREATE:
        return ioctl_tnpu_pool_create(arg);
    default:
        PRINT_TNPU_ERROR("Ioctl does not support %d\n", cmd);
        return -ENOTTY;
    }
    return 0;
}

#ifdef CONFIG_PM
static int tnpu_suspend(struct device *dev) {
    tnpu_clk_close();
    return 0;
}
static int tnpu_resume(struct device *dev) {
    return tnpu_clk_open();
}
#endif // CONFIG_PM


static int __init tnpu_init(void)
{
    int ret;

    // pdev
    tnpu.pdev.name = "tnpu";
    tnpu.pdev.id = 0;
    tnpu.pdev.resource = tnpu.resource;
    tnpu.pdev.num_resources = ARRAY_SIZE(tnpu.resource);
    tnpu.pdev.dev.release = tnpu_device_release;
    // pdrv
    tnpu.pdrv.probe        = tnpu_probe;
    tnpu.pdrv.remove       = tnpu_remove;
    tnpu.pdrv.driver.name  = "tnpu";
    tnpu.pdrv.driver.owner = THIS_MODULE,
#ifdef CONFIG_PM
    tnpu.pdrv.driver.pm = &tnpu.pm_ops,
    tnpu.pm_ops.suspend = tnpu_suspend;
    tnpu.pm_ops.resume  = tnpu_resume;
#endif
    // misc
    tnpu.misc.minor = MISC_DYNAMIC_MINOR;
    tnpu.misc.name  = TNPU_DEV_NAME;
    tnpu.misc.fops  = &tnpu.fops;
    // fops
    tnpu.fops.owner          = THIS_MODULE;
    tnpu.fops.open           = tnpu_open;
    tnpu.fops.release        = tnpu_release;
    tnpu.fops.read           = tnpu_read;
    tnpu.fops.write          = tnpu_write;
    tnpu.fops.mmap           = tnpu_mmap;
    tnpu.fops.unlocked_ioctl = tnpu_ioctl;
    // resource
    tnpu.resource[0].start = TNPU_CCU_PADDR;
    tnpu.resource[0].end   = TNPU_CCU_PADDR + TNPU_CCU_SIZE;
    tnpu.resource[0].flags = IORESOURCE_MEM;
    tnpu.resource[1].start = TNPU_DRAM_PADDR;
    tnpu.resource[1].end   = TNPU_DRAM_PADDR + TNPU_DRAM_SIZE;
    tnpu.resource[1].flags = IORESOURCE_MEM;
    tnpu.resource[2].start = TNPU_IRQ_NUM;
    tnpu.resource[2].end   = TNPU_IRQ_NUM;
    tnpu.resource[2].flags = IORESOURCE_IRQ;

    ret = platform_device_register(&tnpu.pdev);
    if (ret) {
        return ret;
    }
    ret = platform_driver_register(&tnpu.pdrv);
    if (ret) {
        platform_device_unregister(&tnpu.pdev);
        return ret;
    }
    PRINT_TNPU_INFO("Successful insmod, version=0x%x.\n", TNPU_DRIVER_VERSION);
    return 0;
}

static void __exit tnpu_exit(void)
{
    platform_driver_unregister(&tnpu.pdrv);
    platform_device_unregister(&tnpu.pdev);
    PRINT_TNPU_INFO("Successful rmmod.\n");
}

module_init(tnpu_init);
module_exit(tnpu_exit);

MODULE_DESCRIPTION("Ingenic TNPU Driver");
MODULE_AUTHOR("LiuTianyang <rick.tyliu@ingenic.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("20241203");
