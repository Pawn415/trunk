#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
// #include "sys/wait.h"
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>

/***************************************************************
Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
文件名		: ledApp.c
作者	  	: 左忠凯
版本	   	: V1.0
描述	   	: chrdevbase驱测试APP。
其他	   	: 无
使用方法	 ：./ledtest /dev/led  0 关闭LED
             ./ledtest /dev/led  1 打开LED
论坛 	   	: www.openedv.com
日志	   	: 初版V1.0 2019/1/30 左忠凯创建
***************************************************************/

#define LEDOFF 0
#define LEDON 1

/*
 * @description		: main主程序
 * @param - argc 	: argv数组元素个数
 * @param - argv 	: 具体参数
 * @return 			: 0 成功;其他 失败
 */
int main(int argc, char *argv[])
{
    int fd, retvalue;
    char *filename;
    unsigned char databuf[100];
    unsigned char databuf_read[100];
    printf("ledApp start\r\n");
    /* 打开led驱动 */
    fd = open("/dev/newchrled", O_RDWR);
    if (fd < 0)
    {
        printf("file %s open failed!\r\n", argv[1]);
        return -1;
    }

    uint8_t cnt = 0;
    uint8_t data = 0;

    while (1)
    {
        /* 向/dev/led文件写入数据 */
        // sprintf(databuf, "write num:%d", cnt);
        // cnt++;
        // retvalue = write(fd, databuf, 20);
        // if (retvalue < 0)
        // {
        //     printf("LED write Failed!\r\n");
        //     close(fd);
        //     return -1;
        // }

        // // sleep(5);
        // /* 向/dev/led文件写入数据 */
        // retvalue = read(fd, databuf_read, 20);
        // printf("read:%s\r\n", databuf_read);
        // if (retvalue < 0)
        // {
        //     printf("LED read Failed!\r\n");
        //     close(fd);
        //     return -1;
        // }

        retvalue = ioctl(fd, 2);
        if (retvalue == -1)
        {

            printf("ioctl: %s\n", strerror(errno));
        }


        retvalue = ioctl(fd, 1);
        if (retvalue < 0)
        {
            printf("ioctl failed 1");
            return -1;
        }
    }

    retvalue = close(fd); /* 关闭文件 */
    if (retvalue < 0)
    {
        printf("file %s close failed!\r\n", argv[1]);
        return -1;
    }
    return 0;
}
