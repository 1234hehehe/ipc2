#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>

#include "tnpu_ioctl.h"

int main(int argc, char *argv[])
{
    int ret;
    tnpu_read_ratio_t dat;
    fd_set read_fds;
    struct timeval timeout;


    int fd = open("/dev/ingenic-npu", O_RDONLY);
    if (fd < 0) {
        printf("Cannot open tnpu dev: %d\n", fd);
        return fd;
    }

    dat.cmd = TNPU_READ_VERSION;
    ret = read(fd, &dat, sizeof(int));
    if (ret < 0) {
		printf("Cannot get tnpu version: %d\n", ret);
        return ret;
    }

    printf("<TNPU> driver version=0x%x\n\n", (unsigned int)dat.cmd);
    fflush(stdout);

    dat.cmd = TNPU_READ_DUTY_INIT;
    ret = read(fd, &dat, sizeof(int));
    if (ret < 0) {
        printf("Cannot init tnpu status: %d\n", ret);
        return ret;
    }

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        ret = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout);
        if (ret > 0) {
            char ch = getchar();
            if (ch == 'q') {
                break;
            }
        }
        dat.cmd = TNPU_READ_DUTY_SHOW;
        ret = read(fd, &dat, sizeof(tnpu_read_ratio_t));
        if (ret < 0) {
            printf("Cannot get tnpu status: %d\n", ret);
            return ret;
        }

        printf("\033[A\r<TNPU> Duty ratio = %.2f%%  \n", (float)(dat.run * 100) / dat.all);
        fflush(stdout);
    }

    dat.cmd = TNPU_READ_DUTY_STOP;
    ret = read(fd, &dat, sizeof(int));
    close(fd);
    return 0;
}

