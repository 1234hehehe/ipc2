/**
 * @file spl_pad_to_block.c
 * @brief: 当spl len不够一个block时候，补齐0，使足够block整除
 * @author qli, qi.li@ingenic.com
 * @version 1
 * @date 2019-05-10
 *
 * 工具作用：
 * 在scboot中，hash模块需要以64byte作为最小粒度输入，因此spl
 * 需要满足64 bytes的整数倍，当不足64 bytes补齐至能够整除64
 * 的长度。
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define BUF_SIZE (200 * 1024 * sizeof(char))
#define BLOCK_SIZE 64

static int resolve_input_path(const char *path, char *resolved)
{
    if (!path || !*path)
        return -1;
    if (strstr(path, "../") || strstr(path, "/..") || strcmp(path, "..") == 0)
        return -1;
    if (!realpath(path, resolved))
        return -1;

    return 0;
}

/* 当spl len不够一个block时候，补齐0，使足够block整除 */
static int change_spl_len(int len)
{
    int clen = len;
    if(len % 64 != 0) {
        clen = (len / 64 + 1) * 64;
    }
    return clen;
}

int main(int argc, char *argv[])
{
    if(argc != 2) {
        printf("Usage %s [spl_path]\n", argv[0]);
        return -1;
    }
    int clen = 0;
    char resolved_path[PATH_MAX];
    char *spl_path = resolved_path;
    FILE *fp;

    char *spl_buf = (char *)malloc(BUF_SIZE);
    if (!spl_buf) {
        printf("malloc failed\n");
        return -1;
    }
    memset(spl_buf, 0xff, BUF_SIZE);

    if (resolve_input_path(argv[1], resolved_path)) {
        printf("unsafe path: %s\n", argv[1]);
        free(spl_buf);
        return -1;
    }

    fp = fopen(spl_path, "r+");
    if (!fp) {
        printf("can not open %s\n", spl_path);
        free(spl_buf);
        return -1;
    }

    int len = fread(spl_buf, sizeof(char), BUF_SIZE, fp);
    if(len <= 0) {
        printf("can not read %s\n", spl_path);
        goto err;
    }

    clen = change_spl_len(len);
    fseek(fp, 0, SEEK_SET);
    printf("before change len: %d, after change len: %d\n", len , clen);
    fwrite(spl_buf, sizeof(char), clen, fp);
    free(spl_buf);
    fclose(fp);
    return 0;

err:
    free(spl_buf);
    fclose(fp);
    return -1;
}
