
#include<linux/module.h>
#include<linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/device.h>




#define GLOBALEMEM_SIZE 0x1000
#define MEM_CLEAR 0x01
#define GLOBALMEM_MAJOR 230
#define GLOBALEMEM_NAME "globalmem"

static int globalmem_major = GLOBALMEM_MAJOR;
module_param(globalmem_major, int, S_IRUGO);
typedef struct  _globalmem_dev
{
    struct cdev  cdev;
    uint8_t mem[GLOBALEMEM_SIZE];
}globalmem_dev_t;

globalmem_dev_t *globalmem_devp;

static int globalmem_open(struct inode *inode, struct file *filp)
{
    filp->private_data = globalmem_devp;
    return 0;
}

static int globalmem_release(struct inode *inode, struct file *filp)
{

    return 0;
}

static int globalmem_unlock_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    globalmem_dev_t *dev = filp->private_data;
    switch (cmd)
    {
    case MEM_CLEAR:
        memset(dev->mem, 0, GLOBALEMEM_SIZE);
        printk(KERN_INFO " clear memory");
        break;

    default:
        return -EINVAL;
        break;
    }
    return 0;
}

static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    unsigned long p = *ppos;
    unsigned int count = size;
    int ret = 0;
    globalmem_dev_t *dev = filp->private_data;

    if (p >= GLOBALEMEM_SIZE)
        return 0;
    if (count > GLOBALEMEM_SIZE - p)
        count = GLOBALEMEM_SIZE - p;
    if (copy_to_user(buf, &dev->mem + p, count))
        return -EFAULT;
    else
    {
        *ppos += count;
        ret = count;
        printk(KERN_INFO "read %d bytes from %lu \r\n", count, p);
    }
    return ret;
}

static ssize_t  globalmem_write(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    unsigned long p = *ppos;
    unsigned int count = size;
    int ret = 0;
    globalmem_dev_t *dev = filp->private_data;

    if (p >= GLOBALEMEM_SIZE)
        return 0;
    if (count > GLOBALEMEM_SIZE - p)
        count = GLOBALEMEM_SIZE - p;
    if (copy_from_user(&dev->mem + p, buf, count))
        return -EFAULT;
    else
    {
        *ppos += count;
        ret = count;
        printk(KERN_INFO "write %d bytes from %lu \r\n", count, p);
    }
    return ret;
}

static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig)
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
        if ( (unsigned int)(offset) > GLOBALEMEM_SIZE)
        {
            ret = -EINVAL;
            break;
        }
        filp->f_pos =( unsigned int)(offset);
        ret = filp->f_pos;
        break;

    case 1:
        if ((filp->f_pos + offset) > GLOBALEMEM_SIZE)
        {
            ret = -EINVAL;
            break;
        }
        if ((filp->f_pos + offset )< 0)
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
    }
    return ret;
}

static const struct file_operations globalmem_fops =
    {
        .owner = THIS_MODULE,
        .llseek = globalmem_llseek,
        .read = globalmem_read,
        .write = globalmem_write,
        .unlocked_ioctl = globalmem_unlock_ioctl,
        .open = globalmem_open,
        .release = globalmem_release,
};

static int globalmem_setup_cdev(globalmem_dev_t *dev, int index)
{
    int err, devno = MKDEV(globalmem_major, index);
    cdev_init(&dev->cdev, &globalmem_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        printk(KERN_NOTICE "ERROR %d adding globalmem %d", err, index);
    }

}

static int __init globalmem_init(void)
{
    int ret;
    dev_t devno = MKDEV(globalmem_major, 0);
    if (globalmem_major)
    {
        ret = register_chrdev_region(devno, 1, GLOBALEMEM_NAME);
    }
    else
    {
        ret = alloc_chrdev_region(&devno, 0, 1, GLOBALEMEM_NAME);
        globalmem_major = MAJOR(devno);
    }
    if (ret < 0)
        return ret;
    globalmem_devp = kzalloc(sizeof( globalmem_dev_t), GFP_KERNEL);
    if (!globalmem_devp)
    {
        ret = -ENOMEM;
        goto fail_malloc;
    }
    globalmem_setup_cdev(globalmem_major, 0);
    return 0;
fail_malloc:
    unregister_chrdev_region(devno, 1);
    return ret;
}
module_init(globalmem_init);

static void __exit globalmem_exit(void)
{
    cdev_del(&globalmem_devp->cdev);
    kfree(globalmem_devp);
    unregister_chrdev_region(MKDEV(globalmem_major, 0), 1);
}
module_exit(globalmem_exit);
MODULE_AUTHOR("HQL");
MODULE_LICENSE("GPL v2");