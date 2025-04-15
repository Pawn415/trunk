#include "common.h"

#if (HW_INFO == LED_INFO)
hw_info_t virt_hw = {
    .node = "/gpioled",
    .gpio_name = "led-gpio",
};
#elif (HW_INFO == KEY_INFO)
hw_info_t virt_hw = {
    .node = "/key",
    .gpio_name = "key-gpio",
};
#endif

int beep_gpio; /* beep所使用的GPIO编号		*/

virt_dev_t virt; /* beep设备 */

/* 定时器回调函数 */
static void timer_function(unsigned long arg)
{
    struct virt_dev *dev = (struct virt_dev *)arg;
    static int sta = 1;
    int timerperiod;
    unsigned long flags;

    printk("sta:%d \r\n", sta);
    sta++;
    /* 重启定时器 */
    spin_lock_irqsave(&dev->lock, flags);
    timerperiod = dev->timeperiod;
    spin_unlock_irqrestore(&dev->lock, flags);
    // mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->timeperiod));
    complete(&dev->completion);
}


/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int virt_open(struct inode *inode, struct file *filp)
{
    struct virt_dev *dev = container_of(inode->i_cdev, struct virt_dev, cdev);
    filp->private_data = dev; /* 设置私有数据 */
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
static ssize_t virt_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue;
    unsigned char databuf;
    unsigned char beepstat;
    int ret;
    struct virt_dev *dev = filp->private_data;

    retvalue = copy_from_user(&databuf, buf, cnt);
    if (retvalue < 0)
    {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }
    printk("%d \r\n", databuf);
    beepstat = databuf; /* 获取状态值 */
    if (beepstat == BEEPON)
    {
        gpio_set_value(beep_gpio, 0); /* 打开蜂鸣器 */
    }
    else if (beepstat == BEEPOFF)
    {
        gpio_set_value(beep_gpio, 1); /* 关闭蜂鸣器 */
    }

    return 0;
}

static long virt_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

    int ret = 0;
    unsigned long flags;
    unsigned long timerperiod;
    static int index = 0;
    switch (cmd)
    {
    case CREATE_DEV_CMD: /* 关闭定时器 */
        printk("CREATE_DEV_CMD\r\n");

        ret = misc_register(virt.dev_virt[index]);
        if (ret < 0)
        {
            printk("misc device misc_register failed!\r\n");
            return -EFAULT;
        }
        index++;
        break;
    case UNCREATE_DEV_CMD: /* 打开定时器 */
        printk("UNCREATE_DEV_CMD\r\n");
        ret = misc_deregister(virt.dev_virt[index]);
        if (ret < 0)
        {
            printk("misc device misc_deregister failed!\r\n");
            return -EFAULT;
        }
        index--;
        break;

    case CLOSE_CMD: /* 关闭定时器 */
        printk("CLOSE_CMD\r\n");
        del_timer_sync(&virt.timer);
        break;
    case OPEN_CMD: /* 打开定时器 */
        printk("OPEN_CMD\r\n");
        spin_lock_irqsave(&virt.lock, flags);
        timerperiod = 3000;
        spin_unlock_irqrestore(&virt.lock, flags);
        mod_timer(&virt.timer, jiffies + msecs_to_jiffies(timerperiod));
        break;
    case SETPERIOD_CMD: /* 设置定时器周期 */

        spin_lock_irqsave(&virt.lock, flags);
        virt.timeperiod = arg;
        spin_unlock_irqrestore(&virt.lock, flags);
        mod_timer(&virt.timer, jiffies + msecs_to_jiffies(arg));
        printk("SETPERIOD_CMD:%d\r\n", virt.timeperiod);
        break;
    case COMPLETION_CMD: /* 设置定时器周期 */

        spin_lock_irqsave(&virt.lock, flags);
        timerperiod = 1000;
        spin_unlock_irqrestore(&virt.lock, flags);
        mod_timer(&virt.timer, jiffies + msecs_to_jiffies(timerperiod));

        if (0 == wait_for_completion_timeout(&virt.completion, TIMEOUT))
        {
            printk("COMPLETION_CMD timeout\r\n");
        }

        break;

    default:
        break;
    }
    return 0;
}

/* 设备操作函数 */
static struct file_operations virt_fops = {
    .owner = THIS_MODULE,
    .open = virt_open,
    .write = virt_write,
    .unlocked_ioctl = virt_unlocked_ioctl,
};

/* MISC设备结构体 */
static struct miscdevice virt_dev = {
    .minor = VIRT_DEV_MINOR,
    .name = "virt",
    .fops = &virt_fops};

/*
 * @description     : flatform驱动的probe函数，当驱动与
 *                    设备匹配以后此函数就会执行
 * @param - dev     : platform设备
 * @return          : 0，成功;其他负值,失败
 */
static void base_init(void)
{
    printk("virt_dev build time:%s %s\n", __DATE__, __TIME__);
    spin_lock_init(&virt.lock);
    init_completion(&virt.completion);
}
static int virt_probe(struct platform_device *dev)
{
    int ret = 0;
    base_init();
    /* 设置BEEP所使用的GPIO */
    /* 1、获取设备节点：beep */
    virt.nd = of_find_node_by_path(virt_hw.node);
    if (virt.nd == NULL)
    {
        printk("beep node not find!\r\n");
        return -EINVAL;
    }

    /* 2、 获取设备树中的gpio属性，得到BEEP所使用的BEEP编号 */
    beep_gpio = of_get_named_gpio(virt.nd, virt_hw.gpio_name, 0);
    if (beep_gpio < 0)
    {
        printk("can't get led-gpio");
        return -EINVAL;
    }

    /* 3、设置GPIO5_IO01为输出，并且输出高电平，默认关闭BEEP */
    ret = gpio_direction_output(beep_gpio, 1);
    if (ret < 0)
    {
        printk("can't set gpio!\r\n");
    }

    ret = misc_register(&virt_dev);
    if (ret < 0)
    {
        printk("misc device register failed!\r\n");
        return -EFAULT;
    }

    /* 6、初始化timer，设置定时器处理函数,还未设置周期，所有不会激活定时器 */
    init_timer(&virt.timer);
    virt.timer.function = timer_function;
    virt.timer.data = (unsigned long)&virt;
    virt.timeperiod = 2000; /* 默认周期为2s */
    // mod_timer(&virt.timer, jiffies + msecs_to_jiffies(virt.timeperiod));

    for (virt.num = 0; virt.num < MAX_NUM_DEV; virt.num++)
    {
        virt.dev_virt[virt.num] = kzalloc(sizeof(struct miscdevice), GFP_KERNEL);
        if (!virt.dev_virt[virt.num])
        {
            printk("kzalloc failed\r\n");
            return -ENOMEM;
        }
        virt.dev_virt[virt.num]->minor = VIRT_DEV_MINOR + virt.num + 1;
        sprintf(&virt.dev_virt_name[virt.num][0], "virt%d", virt.num);
        virt.dev_virt[virt.num]->name = &virt.dev_virt_name[virt.num][0];
        virt.dev_virt[virt.num]->fops = &virt_fops;
    }
    virt.num = 0;
    return 0;
}

/*
 * @description     : platform驱动的remove函数，移除platform驱动的时候此函数会执行
 * @param - dev     : platform设备
 * @return          : 0，成功;其他负值,失败
 */
static int virt_remove(struct platform_device *dev)
{
    /* 注销设备的时候关闭LED灯 */
    int ret;
    gpio_set_value(beep_gpio, 1);
    del_timer_sync(&virt.timer); /* 删除timer */
    ret = misc_deregister(&virt_dev);
    if (ret < 0)
    {
        printk("misc device register failed!\r\n");
        return -EFAULT;
    }

    for (virt.num = 0; virt.num < MAX_NUM_DEV; virt.num++)
    {
        kfree(virt.dev_virt[virt.num]);
        virt.dev_virt[virt.num] = NULL;
    }
    return 0;
}

/* 匹配列表 */
static const struct of_device_id virt_of_match[] = {
    {.compatible = COMPATIBLE},
    {/* Sentinel */},
};

/* platform驱动结构体 */
static struct platform_driver virt_driver = {
    .driver = {
        .name = DRIVER_NAME,             /* 驱动名字，用于和设备匹配 */
        .of_match_table = virt_of_match, /* 设备树匹配表          */
    },
    .probe = virt_probe,
    .remove = virt_remove,
};

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init virt_init(void)
{
    return platform_driver_register(&virt_driver);
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit virt_exit(void)
{
    platform_driver_unregister(&virt_driver);
}

module_init(virt_init);
module_exit(virt_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("hql");
