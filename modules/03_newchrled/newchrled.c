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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/slab.h>

#define NEWCHRLED_CNT 1            /* 设备号个数 */
#define NEWCHRLED_NAME "newchrled" /* 名字 */
/* newchrled设备结构体 */
typedef struct newchrled_dev
{
    dev_t devid;           /* 设备号 	 */
    struct cdev cdev;      /* cdev 	*/
    struct class *class;   /* 类 		*/
    struct device *device; /* 设备 	 */
    int major;             /* 主设备号	  */
    int minor;             /* 次设备号   */
    char mem[200];

} newchrled_dev_t;

newchrled_dev_t *dev; /* led设备 */
/*

 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int led_open(struct inode *inode, struct file *filp)
{
    filp->private_data = dev; /* 设置私有数据 */
    return 0;
}

/*
 * @description		: 从设备读取数据
 * @param - filp 	: 要打开的设备文件(文件描述符)
 * @param - buf 	: 返回给用户空间的数据缓冲区
 * @param - cnt 	: 要读取的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 读取的字节数，如果为负值，表示读取失败
 */

static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue = copy_to_user(buf, dev->mem, cnt);
    if (retvalue < 0)
    {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }
    return cnt;
}

/*
 * @description		: 向设备写数据
 * @param - filp 	: 设备文件，表示打开的文件描述符
 * @param - buf 	: 要写给设备写入的数据
 * @param - cnt 	: 要写入的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 写入的字节数，如果为负值，表示写入失败
 */

static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue = copy_from_user(dev->mem, buf, cnt);
    if (retvalue < 0)
    {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }
    printk("%s\r\n", dev->mem);
    return cnt;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */

static int led_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static long globalmem_unlock_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    printk(" globalmem_unlock_ioctl\r\n");
    switch (cmd)
    {
    case 1:
        printk(" ioctrl 1\r\n");
        break;
    case 2:
        printk(" ioctrl 2\r\n");
        break;
    default:
        return -EINVAL;
        break;
    }
    return 0;
}

/* 设备操作函数 */

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
    .unlocked_ioctl = globalmem_unlock_ioctl,
};

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
typedef void (*pvoid)(void);
static int __init led_init(void)
{
    /* 注册字符设备驱动 */
    dev = kmalloc(sizeof(newchrled_dev_t), GFP_KERNEL);
    /* 1、创建设备号 */
    if (dev->major)
    { /*  定义了设备号 */
        dev->devid = MKDEV(dev->major, 0);
        register_chrdev_region(dev->devid, NEWCHRLED_CNT, NEWCHRLED_NAME);
    }
    else
    {                                                                       /* 没有定义设备号 */
        alloc_chrdev_region(&dev->devid, 0, NEWCHRLED_CNT, NEWCHRLED_NAME); /* 申请设备号 */
        dev->major = MAJOR(dev->devid);                                     /* 获取分配号的主设备号 */
        dev->minor = MINOR(dev->devid);                                     /* 获取分配号的次设备号 */
    }
    printk("newcheled major=%d,minor=%d\r\n", dev->major, dev->minor);
    /* 2、初始化cdev */
    dev->cdev.owner = THIS_MODULE;
    cdev_init(&dev->cdev, &fops);
    /* 3、添加一个cdev */
    cdev_add(&dev->cdev, dev->devid, NEWCHRLED_CNT);
    /* 4、创建类 */
    dev->class = class_create(THIS_MODULE, NEWCHRLED_NAME);
    if (IS_ERR(dev->class))
    {
        return PTR_ERR(dev->class);
    }
    /* 5、创建设备 */
    dev->device = device_create(dev->class, NULL, dev->devid, NULL, NEWCHRLED_NAME);
    if (IS_ERR(dev->device))
    {
        return PTR_ERR(dev->device);
    }
    return 0;
}

/*

 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */

static void __exit led_exit(void)
{
    /* 注销字符设备驱动 */
    cdev_del(&dev->cdev);                                /*  删除cdev */
    unregister_chrdev_region(dev->devid, NEWCHRLED_CNT); /* 注销设备号 */
    device_destroy(dev->class, dev->devid);
    class_destroy(dev->class);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zuozhongkai");
