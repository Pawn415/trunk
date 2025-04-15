#ifndef __COMMON_H_
#define __COMMON_H_

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/semaphore.h>
#include <linux/completion.h>
#include <linux/of_irq.h>
#include <linux/irq.h>

#define LED_INFO 0
#define KEY_INFO 1
#define HW_INFO LED_INFO

#define TYPE 'C'
#define CREATE_DEV_CMD _IOWR(TYPE, 0, int)
#define UNCREATE_DEV_CMD _IOWR(TYPE, 1, int)

#define CLOSE_CMD _IOWR(TYPE, 10, int)
#define OPEN_CMD _IOWR(TYPE, 11, int)
#define SETPERIOD_CMD _IOWR(TYPE, 12, int)

#define COMPLETION_CMD _IOWR(TYPE, 23, int)

#define VIRT_DEV_NAME "virt" /* 名字 	*/
#define VIRT_DEV_MINOR 144   /* 子设备号 */
#define BEEPOFF 0            /* 关蜂鸣器 */
#define BEEPON 1             /* 开蜂鸣器 */
#define TIMEOUT 3000

#define MAX_NUM_DEV 10
/* virt设备结构体 */

#define KEY_NUM 1
typedef struct virt_dev
{
    dev_t devid;            /* 设备号 	 */
    struct cdev cdev;       /* cdev 	*/
    struct class *class;    /* 类 		*/
    struct device *device;  /* 设备 	 */
    struct device_node *nd; /* 设备节点 */
                            // 按键中断
    // atomic_t keyvalue;                      /* 有效的按键键值 */
    // atomic_t releasekey;                    /* 标记是否完成一次完成的按键，包括按下和释放 */
    // struct irq_keydesc irqkeydesc[KEY_NUM]; /* 按键描述数组 */
    // uint8_t curkeynum;                      /* 当前的按键号 */
    // 完成量
    struct completion completion; /*完成量*/

    // timer参数
    int timeperiod;          /* 定时周期,单位为ms */
    struct timer_list timer; /* 定义一个定时器*/
    spinlock_t lock;         /* 定义自旋锁 */
                             // 多个设备
    struct miscdevice *dev_virt[MAX_NUM_DEV];
    uint16_t num;
    char dev_virt_name[10][10];
} virt_dev_t;

typedef struct hw_info
{
    char *node;
    char *gpio_name;
    char *compatible;
    char *driver_name;
} hw_info_t;

#if (HW_INFO == LED_INFO)
#define COMPATIBLE "atkalpha-gpioled"
#define DRIVER_NAME "imx6ul-led"
#elif (HW_INFO == KEY_INFO)
#define COMPATIBLE "atkalpha-gpioled"
#define DRIVER_NAME "imx6ul-led"
#endif // DEBUG

#endif // !1
