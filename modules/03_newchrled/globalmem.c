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

#define NEWCHRLED_CNT 1            /* 设备号个数 */
#define NEWCHRLED_NAME "newchrled" /* 名字 */
#define GLOBALEMEM_SIZE 200

/* newchrled设备结构体 */
typedef struct _newchrled_dev
{
    dev_t devid;           /* 设备号 	 */
    struct cdev cdev;      /* cdev 	*/
    struct class *class;   /* 类 		*/
    struct device *device; /* 设备 	 */
    int major;             /* 主设备号	  */
    int minor;             /* 次设备号   */
    uint8_t mem[200];
} newchrled_dev_t;

newchrled_dev_t *newchrled; /* led设备 */

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int led_open(struct inode *inode, struct file *filp)
{
    filp->private_data = newchrled; /* 设置私有数据 */
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
    unsigned long p = *offt;
    unsigned int count = cnt;
    int ret = 0;
    newchrled_dev_t *dev = filp->private_data;

    if (p >= GLOBALEMEM_SIZE)
        return 0;
    if (count > GLOBALEMEM_SIZE - p)
        count = GLOBALEMEM_SIZE - p;
    if (copy_to_user(buf, &dev->mem + p, count))
        return -EFAULT;
    else
    {
        *offt += count;
        ret = count;
        printk(KERN_INFO "read %d bytes from %lu \r\n", count, p);
    }
    return ret;
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
    unsigned long p = *offt;
    unsigned int count = cnt;
    int ret = 0;
    newchrled_dev_t *dev = filp->private_data;

    if (p >= GLOBALEMEM_SIZE)
        return 0;
    if (count > GLOBALEMEM_SIZE - p)
        count = GLOBALEMEM_SIZE - p;
    if (copy_from_user(&dev->mem + p, buf, count))
        return -EFAULT;
    else
    {
        *offt += count;
        ret = count;
        printk(KERN_INFO "write %d bytes from %lu \r\n", count, p);
    }
    return ret;
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



static loff_t led_llseek(struct file *filp, loff_t offset, int orig)
{
    loff_t ret = 0;
    switch (orig)
    {
    case 0:
        if (offset < 0)
        {
            ret = -EINVAL;
            break;
        }
        if ((unsigned int)(offset) > GLOBALEMEM_SIZE)
        {
            ret = -EINVAL;
            break;
        }
        filp->f_pos = (unsigned int)(offset);
        ret = filp->f_pos;
        break;

    case 1:
        if ((filp->f_pos + offset) > GLOBALEMEM_SIZE)
        {
            ret = -EINVAL;
            break;
        }
        if (filp->f_pos + offset < 0)
        {
            ret = -EINVAL;
            break;
        }
        filp->f_pos += offset;
        ret = filp->f_pos;
        break;

    default:
        ret = -EINVAL;
        break;
        break;
    }
    return ret;
}

static int led_unlock_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    newchrled_dev_t *dev = filp->private_data;
    switch (cmd)
    {
    case 1:
        memset(dev->mem, 0, GLOBALEMEM_SIZE);
        printk(KERN_INFO " clear memory");
        break;

    default:
        return -EINVAL;
        break;
    }
    return 0;
}

/* 设备操作函数 */
static struct file_operations newchrled_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
    .llseek = led_llseek,
    .unlocked_ioctl = led_unlock_ioctl,

};

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init led_init(void)
{

    /* 注册字符设备驱动 */
    /* 1、创建设备号 */
    if (newchrled->major)
    { /*  定义了设备号 */
        newchrled->devid = MKDEV(newchrled->major, 0);
        register_chrdev_region(newchrled->devid, NEWCHRLED_CNT, NEWCHRLED_NAME);
    }
    else
    {                                                                             /* 没有定义设备号 */
        alloc_chrdev_region(&newchrled->devid, 0, NEWCHRLED_CNT, NEWCHRLED_NAME); /* 申请设备号 */
        newchrled->major = MAJOR(newchrled->devid);                               /* 获取分配号的主设备号 */
        newchrled->minor = MINOR(newchrled->devid);                               /* 获取分配号的次设备号 */
    }
    printk("newcheled major=%d,minor=%d\r\n", newchrled->major, newchrled->minor);

    /* 2、初始化cdev */
    newchrled->cdev.owner = THIS_MODULE;
    cdev_init(&newchrled->cdev, &newchrled_fops);

    /* 3、添加一个cdev */
    cdev_add(&newchrled->cdev, newchrled->devid, NEWCHRLED_CNT);

    /* 4、创建类 */
    newchrled->class = class_create(THIS_MODULE, NEWCHRLED_NAME);
    if (IS_ERR(newchrled->class))
    {
        return PTR_ERR(newchrled->class);
    }

    /* 5、创建设备 */
    newchrled->device = device_create(newchrled->class, NULL, newchrled->devid, NULL, NEWCHRLED_NAME);
    if (IS_ERR(newchrled->device))
    {
        return PTR_ERR(newchrled->device);
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
    cdev_del(&newchrled->cdev);                                /*  删除cdev */
    unregister_chrdev_region(newchrled->devid, NEWCHRLED_CNT); /* 注销设备号 */

    device_destroy(newchrled->class, newchrled->devid);
    class_destroy(newchrled->class);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("HQL");
