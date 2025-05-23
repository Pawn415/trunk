#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
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
/***************************************************************
Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
文件名		: misc_dev.c
作者	  	: 左忠凯
版本	   	: V1.0
描述	   	: 采用MISC的蜂鸣器驱动程序。
其他	   	: 无
论坛 	   	: www.openedv.com
日志	   	: 初版V1.0 2019/8/20 左忠凯创建
***************************************************************/
#define misc_dev_NAME		"misc_dev"	/* 名字 	*/
#define misc_dev_MINOR		144			/* 子设备号 */


/* misc_dev设备结构体 */
struct misc_dev_dev{
	dev_t devid;			/* 设备号 	 */
	struct cdev cdev;		/* cdev 	*/
	struct class *class;	/* 类 		*/
	struct device *device;	/* 设备 	 */
	struct device_node	*nd; /* 设备节点 */
	int beep_gpio;			/* beep所使用的GPIO编号		*/
};

struct misc_dev_dev misc_dev;		/* beep设备 */

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int misc_dev_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &misc_dev; /* 设置私有数据 */
	return 0;
}

/*
 * @description		: 向设备写数据 
 * @param - filp 	: 设备文件，表示打开的文件描述符
 * @param - buf 	: 要写给设备写入的数据
 * @param - cnt 	: 要写入的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t misc_dev_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue;
	unsigned char databuf[256];
	struct misc_dev_dev *dev = filp->private_data;

	retvalue = copy_from_user(databuf, buf, cnt);
	if(retvalue < 0) {
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}

    printk("misc_dev_write:%s\r\n",databuf);
	return cnt;
}

/* 设备操作函数 */
static struct file_operations misc_dev_fops = {
	.owner = THIS_MODULE,
	.open = misc_dev_open,
	.write = misc_dev_write,
};

/* MISC设备结构体 */
static struct miscdevice beep_miscdev = {
	.minor = misc_dev_MINOR,
	.name = misc_dev_NAME,
	.fops = &misc_dev_fops,
};

 /*
  * @description     : flatform驱动的probe函数，当驱动与
  *                    设备匹配以后此函数就会执行
  * @param - dev     : platform设备
  * @return          : 0，成功;其他负值,失败
  */
static int misc_dev_probe(struct platform_device *dev)
{
	int ret = 0;


	
	/* 一般情况下会注册对应的字符设备，但是这里我们使用MISC设备
  	 * 所以我们不需要自己注册字符设备驱动，只需要注册misc设备驱动即可
	 */
	ret = misc_register(&beep_miscdev);
	if(ret < 0){
		printk("misc device register failed!\r\n");
		return -EFAULT;
	}

	return 0;
}

/*
 * @description     : platform驱动的remove函数，移除platform驱动的时候此函数会执行
 * @param - dev     : platform设备
 * @return          : 0，成功;其他负值,失败
 */
static int misc_dev_remove(struct platform_device *dev)
{

	/* 注销misc设备 */
	misc_deregister(&beep_miscdev);
	return 0;
}


 
 /* platform驱动结构体 */
static struct platform_driver beep_driver = {
     .driver     = {
        //  .name   = "imx6ul-beep",         /* 驱动名字，用于和设备匹配 */
        //  .of_match_table = beep_of_match, /* 设备树匹配表          */
     },
     .probe      = misc_dev_probe,
     .remove     = misc_dev_remove,
};

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init misc_dev_init(void)
{
	return platform_driver_register(&beep_driver);
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit misc_dev_exit(void)
{
	platform_driver_unregister(&beep_driver);
}

module_init(misc_dev_init);
module_exit(misc_dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("hql");
