#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/slab.h>

#define MYFS_MAGIC 0x20240524

/*
 * inode 私有数据
 */
struct ktrap_inode_info {
    char *data;
    size_t size;
};

static struct file_system_type ktrap_type;
static const struct super_operations ktrap_super_ops;
static const struct inode_operations ktrap_dir_inode_ops;
static const struct file_operations ktrap_file_ops;
static const struct inode_operations ktrap_file_inode_ops;

/**********************************************************
 * 文件读
 **********************************************************/
static ssize_t ktrap_read(struct file *filp,
                         char __user *buf,
                         size_t len,
                         loff_t *ppos)
{
    struct inode *inode = file_inode(filp);
    struct ktrap_inode_info *info = inode->i_private;

    return simple_read_from_buffer(buf,
                                   len,
                                   ppos,
                                   info->data,
                                   info->size);
}

/**********************************************************
 * 文件写
 **********************************************************/
static ssize_t ktrap_write(struct file *filp,
                          const char __user *buf,
                          size_t len,
                          loff_t *ppos)
{
    struct inode *inode = file_inode(filp);
    struct ktrap_inode_info *info = inode->i_private;

    kfree(info->data);

    info->data = kzalloc(len + 1, GFP_KERNEL);
    if (!info->data)
        return -ENOMEM;

    if (copy_from_user(info->data, buf, len))
        return -EFAULT;

    info->size = len;

    *ppos = len;

    inode->i_size = len;

    return len;
}

/**********************************************************
 * 创建 inode
 **********************************************************/
static struct inode *ktrap_get_inode(struct super_block *sb,
                                    const struct inode *dir,
                                    umode_t mode)
{
    struct inode *inode;

    inode = new_inode(sb);
    if (!inode)
        return NULL;

    inode->i_ino = get_next_ino();

    inode_init_owner(inode, dir, mode);

    inode->i_atime = inode->i_mtime = inode->i_ctime =
        current_fs_time(sb);

    if (S_ISDIR(mode)) {
        inode->i_op = &ktrap_dir_inode_ops;
        inode->i_fop = &simple_dir_operations;

        inc_nlink(inode);
    } else if (S_ISREG(mode)) {

        struct ktrap_inode_info *info;

        info = kzalloc(sizeof(*info), GFP_KERNEL);
        if (!info) {
            iput(inode);
            return NULL;
        }

        inode->i_private = info;

        inode->i_op = &ktrap_file_inode_ops;
        inode->i_fop = &ktrap_file_ops;
    }

    return inode;
}

/**********************************************************
 * create
 **********************************************************/
 static int ktrap_create(struct inode *dir,
    struct dentry *dentry,
    umode_t mode,
    bool excl)
{
struct inode *inode;

inode = ktrap_get_inode(dir->i_sb, dir, mode);
if (!inode)
return -ENOMEM;

/*
* 不要 d_add()
* dentry 已经由 simple_lookup hash 过
*/
d_instantiate(dentry, inode);

dget(dentry);

dir->i_mtime = dir->i_ctime =
current_fs_time(dir->i_sb);

return 0;
}

/**********************************************************
 * lookup
 **********************************************************/
static struct dentry *ktrap_lookup(struct inode *parent_inode,
                                  struct dentry *child_dentry,
                                  unsigned int flags)
{
    return simple_lookup(parent_inode, child_dentry, flags);
}

/**********************************************************
 * inode ops
 **********************************************************/
static const struct inode_operations ktrap_dir_inode_ops = {
    .lookup = ktrap_lookup,
    .create = ktrap_create,
};

static const struct inode_operations ktrap_file_inode_ops = {
};

/**********************************************************
 * file ops
 **********************************************************/
static const struct file_operations ktrap_file_ops = {
    .read = ktrap_read,
    .write = ktrap_write,
    .llseek = default_llseek,
};

/**********************************************************
 * super block
 **********************************************************/
static const struct super_operations ktrap_super_ops = {
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
};

/**********************************************************
 * fill super
 **********************************************************/
static int ktrap_fill_super(struct super_block *sb,
                           void *data,
                           int silent)
{
    struct inode *root_inode;
    struct dentry *root;

    sb->s_magic = MYFS_MAGIC;
    sb->s_op = &ktrap_super_ops;

    root_inode = ktrap_get_inode(sb, NULL, S_IFDIR | 0755);
    if (!root_inode)
        return -ENOMEM;

    root = d_make_root(root_inode);
    if (!root)
        return -ENOMEM;

    sb->s_root = root;

    return 0;
}

/**********************************************************
 * mount
 **********************************************************/
static struct dentry *ktrap_mount(struct file_system_type *fs_type,
                                 int flags,
                                 const char *dev_name,
                                 void *data)
{
    return mount_nodev(fs_type,
                       flags,
                       data,
                       ktrap_fill_super);
}

/**********************************************************
 * filesystem type
 **********************************************************/
static struct file_system_type ktrap_type = {
    .owner = THIS_MODULE,
    .name = "ktrap",
    .mount = ktrap_mount,
    .kill_sb = kill_litter_super,
};

/**********************************************************
 * init
 **********************************************************/
static int __init ktrap_init(void)
{
    int ret;

    ret = register_filesystem(&ktrap_type);

    if (ret == 0)
        printk(KERN_INFO "ktrap: registered\n");

    return ret;
}

/**********************************************************
 * exit
 **********************************************************/
static void __exit ktrap_exit(void)
{
    unregister_filesystem(&ktrap_type);

    printk(KERN_INFO "ktrap: unregistered\n");
}

module_init(ktrap_init);
module_exit(ktrap_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChatGPT");
MODULE_DESCRIPTION("Simple MyFS for Linux 4.1.15");