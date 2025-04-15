#include "common.h"

extern void build_time_printf(void);
extern void printf_b(void);

#define BEEPOFF 0
#define BEEPON 1

#define WRITE_CMD 0
#define IOCTRL_CMD 1
/*
 * @description		: main主程序
 * @param - argc 	: argv数组元素个数
 * @param - argv 	: 具体参数
 * @return 			: 0 成功;其他 失败
 */
int main(int argc, char *argv[])
{

    build_time_printf();
    // printf_b();

    int fd, ret;
    unsigned int cmd;
    unsigned int arg;
    char *filename;
    unsigned char databuf[1];

// #ifdef DEFINE_TEST
//     printf(">>>>Receive define from makefile!\n");
// #else
//     printf(">>>>NOT Receive define from makefile!\n", DEFINE_TEST);
// #endif
//     printf(">>>>Make Time: %s  \n", MAKE_TIME);

    if (argc == 1)
    {
        printf("./virtApp /dev/virt w 1\n\
./virtApp /dev/virt i 0 del dev\n\
./virtApp /dev/virt i 1 create dev\n\
./virtApp /dev/virt i 2 close timer\n\
./virtApp /dev/virt i 3 opne timer\n\
./virtApp /dev/virt i 4 set period(ms)\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename, O_RDWR); /* 打开beep驱动 */
    if (fd < 0)
    {
        printf("file %s open failed!\r\n", argv[1]);
        return -1;
    }

    if (0 == strcmp(argv[2], "w"))
    {
        databuf[0] = atoi(argv[3]); /* 要执行的操作：打开或关闭 */
        ret = write(fd, databuf, sizeof(databuf));
        if (ret < 0)
        {
            printf("LED Control Failed!\r\n");
            close(fd);
            return -1;
        }
    }
    else if (0 == strcmp(argv[2], "i"))
    {

        cmd = atoi(argv[3]);
        switch (cmd)
        {
        case 0:
            cmd = UNCREATE_DEV_CMD;
            ret = ioctl(fd, cmd, 0);
            break;
        case 1:
            cmd = CREATE_DEV_CMD;
            ret = ioctl(fd, cmd, 0);
            break;
        case 2:
            cmd = CLOSE_CMD;
            ret = ioctl(fd, cmd, 0);
            break;
        case 3:
            cmd = OPEN_CMD;
            ret = ioctl(fd, cmd, 0);
            break;
        case 4:
            cmd = SETPERIOD_CMD;
            ret = ioctl(fd, cmd, atoi(argv[4]));
            break;
        case 5:
            cmd = COMPLETION_CMD;
            ret = ioctl(fd, cmd, 0);
            break;

        default:
            break;
        }
        if (ret < 0)
        {
            printf("ioctl error:%s\n", strerror(errno));
            printf("Call cmd virt_App fail:%d\n", ret);
            return -1;
        }
    }

    ret = close(fd); /* 关闭文件 */
    if (ret < 0)
    {
        printf("close error:%s\n", strerror(errno));
        printf("file %s close failed!\r\n", argv[1]);
        return -1;
    }
    printf("close\n");
    return 0;
}
