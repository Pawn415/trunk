/*
 * data_breakpoint.c - Sample HW Breakpoint file to watch kernel data address
 */
 #include <linux/module.h>
 #include <linux/kernel.h>
 #include <linux/init.h>
 #include <linux/kallsyms.h>
 #include <linux/proc_fs.h>
 #include <linux/uaccess.h>
 #include <linux/slab.h>
 #include <linux/mutex.h>
 #include <linux/delay.h>  // 添加 delay.h 头文件
 #include <linux/io.h>
 #include <linux/perf_event.h>
 #include <linux/hw_breakpoint.h>
 
 struct perf_event * __percpu *sample_hbp;
 
 static struct proc_dir_entry *proc_entry;
 static unsigned long target_addr = 0;
 static DEFINE_MUTEX(bp_mutex);    /* Protect breakpoint operations */
 
 /* 检查硬件断点能力 */
 static void check_hw_breakpoint_capability(void)
 {
     printk(KERN_INFO "=== HW Breakpoint Capability Check ===\n");
     printk(KERN_INFO "Available data breakpoint slots: %d\n", hw_breakpoint_slots(TYPE_DATA));
     printk(KERN_INFO "======================================\n");
 }
 
 /* 验证目标地址 */
 static int validate_target_address(unsigned long addr)
 {
     struct page *page;
     
     /* 检查地址是否在内核空间 */
     if (addr < PAGE_OFFSET) {
         printk(KERN_ERR "Address 0x%lx is in userspace (PAGE_OFFSET=0x%lx)\n", 
                addr, PAGE_OFFSET);
         return -EINVAL;
     }
     
     printk(KERN_INFO "Address 0x%lx is in kernel space\n", addr);
     
     /* 尝试获取对应的物理页 */
     page = virt_to_page((void *)addr);
     if (!page) {
         printk(KERN_WARNING "Cannot get physical page for address 0x%lx\n", addr);
         /* 这不一定是错误，对于某些特殊映射可能无法获取物理页 */
     } else {
         printk(KERN_INFO "Address 0x%lx maps to physical page %p\n", addr, page);
     }
     
     /* 检查地址对齐 */
     if (addr & 0x3) {
         printk(KERN_ERR "Address 0x%lx is not 4-byte aligned\n", addr);
         return -EINVAL;
     }
     
     return 0;
 }
 
 static void sample_hbp_handler(struct perf_event *bp,
                    struct perf_sample_data *data,
                    struct pt_regs *regs)
 {
     printk(KERN_INFO "*** HW Breakpoint Triggered ***\n");
     printk(KERN_INFO "Virtual address 0x%lx is accessed\n", target_addr);
     
     /* 使用架构无关的方式获取指令指针 */
 #ifdef CONFIG_X86
     printk(KERN_INFO "Instruction pointer: 0x%lx\n", regs->ip);
 #elif defined(CONFIG_ARM) || defined(CONFIG_ARM64)
    //  printk(KERN_INFO "Program counter: 0x%lx\n", regs->pc);
 #else
     printk(KERN_INFO "Architecture specific instruction pointer\n");
 #endif
     
     dump_stack();
     printk(KERN_INFO "*** End of Breakpoint Handler ***\n");
 }
 
 /* Remove current breakpoint if exists */
 static void remove_breakpoint(void)
 {
     if (sample_hbp) {
         unregister_wide_hw_breakpoint(sample_hbp);
         sample_hbp = NULL;
         printk(KERN_INFO "HW Breakpoint removed\n");
     }
 }
 
 /* Setup new breakpoint with current target_addr */
 static int setup_breakpoint(void)
 {
     int ret;
     struct perf_event_attr attr;
 
     if (target_addr == 0) {
         printk(KERN_INFO "No target address set\n");
         return -EINVAL;
     }
 
     printk(KERN_INFO "Setting HW breakpoint at 0x%lx\n", target_addr);
 
     /* 验证地址 */
     ret = validate_target_address(target_addr);
     if (ret) {
         printk(KERN_ERR "Invalid target address 0x%lx\n", target_addr);
         return ret;
     }
 
     /* Initialize breakpoint attributes */
     hw_breakpoint_init(&attr);
     attr.bp_addr = target_addr;
     attr.bp_len = HW_BREAKPOINT_LEN_4;
     attr.bp_type = HW_BREAKPOINT_W | HW_BREAKPOINT_R;
 
     /* 使用正确的格式说明符 */
     printk(KERN_INFO "Breakpoint attributes: addr=0x%llx, len=%d, type=%d\n", 
            (unsigned long long)attr.bp_addr, (int)attr.bp_len, (int)attr.bp_type);
 
     /* Register the breakpoint */
     sample_hbp = register_wide_hw_breakpoint(&attr, sample_hbp_handler, NULL);
     if (IS_ERR((void __force *)sample_hbp)) {
         ret = PTR_ERR((void __force *)sample_hbp);
         sample_hbp = NULL;
         printk(KERN_ERR "Breakpoint registration failed for address 0x%lx, error: %d\n", 
                target_addr, ret);
         
         /* 检查具体错误原因 */
         if (ret == -ENOSPC)
             printk(KERN_ERR "No available HW breakpoint registers\n");
         else if (ret == -EINVAL)
             printk(KERN_ERR "Invalid breakpoint parameters\n");
         else if (ret == -ENOMEM)
             printk(KERN_ERR "Memory allocation failed\n");
         else
             printk(KERN_ERR "Unknown error: %d\n", ret);
             
         return ret;
     }
 
     printk(KERN_INFO "HW Breakpoint for address 0x%lx installed successfully\n", target_addr);
     return 0;
 }
 
 /* 测试断点触发 */
 static void test_breakpoint_trigger(unsigned long addr)
 {
     volatile unsigned long *test_ptr = (volatile unsigned long *)addr;
     unsigned long value;
     
     printk(KERN_INFO "=== Testing Breakpoint Trigger ===\n");
     
     /* 测试读操作 */
     printk(KERN_INFO "Attempting read from 0x%lx\n", addr);
     value = ioread32(test_ptr);
     printk(KERN_INFO "Read value: 0x%lx\n", value);
     
     /* 短暂延迟 - 使用 msleep 替代 mdelay */
     msleep(10);
     value++;
     /* 测试写操作 */
     printk(KERN_INFO "Attempting write to 0x%lx\n", addr);
     iowrite32(value, test_ptr);
     printk(KERN_INFO "Write completed\n");
     
     printk(KERN_INFO "=== Test Completed ===\n");
 }
 
 /* Proc file read operation */
 static ssize_t bp_proc_read(struct file *file, char __user *buf, 
                 size_t count, loff_t *ppos)
 {
     char info[512];
     int len;
     
     if (*ppos > 0)
         return 0;
 
     if (target_addr == 0) {
         len = snprintf(info, sizeof(info), 
                       "No breakpoint set\n"
                       "Usage:\n"
                       "  echo <hex_address> > /proc/see  # Set breakpoint\n"
                       "  echo test > /proc/see           # Test current breakpoint\n"
                       "  echo clear > /proc/see          # Clear breakpoint\n"
                       "  echo status > /proc/see         # Show status\n"
                       "  cat /proc/see                   # Check status\n");
     } else if (sample_hbp) {
         len = snprintf(info, sizeof(info), 
                    "Active breakpoint at: 0x%lx\n"
                    "Breakpoint type: Read/Write\n"
                    "Breakpoint length: 4 bytes\n"
                    "Status: Monitoring\n",
                    target_addr);
     } else {
         len = snprintf(info, sizeof(info), 
                    "Target address: 0x%lx (breakpoint not active)\n"
                    "Use 'echo test' to activate breakpoint\n", 
                    target_addr);
     }
 
     if (copy_to_user(buf, info, len))
         return -EFAULT;
 
     *ppos = len;
     return len;
 }
 
 /* Proc file write operation */
 static ssize_t bp_proc_write(struct file *file, const char __user *buf,
                   size_t count, loff_t *ppos)
 {
     char input[32];
     unsigned long new_addr;
     int ret;
 
     if (count >= sizeof(input))
         return -EINVAL;
 
     if (copy_from_user(input, buf, count))
         return -EFAULT;
 
     input[count] = '\0';
 
     /* Remove trailing newline */
     if (count > 0 && input[count - 1] == '\n')
         input[count - 1] = '\0';
 
     mutex_lock(&bp_mutex);
 
     /* 检查命令 */
     if (strcmp(input, "test") == 0) {
         if (target_addr == 0) {
             printk(KERN_ERR "No target address set for testing\n");
             mutex_unlock(&bp_mutex);
             return -EINVAL;
         }
         
         /* 如果断点不存在，先设置 */
         if (!sample_hbp) {
             ret = setup_breakpoint();
             if (ret) {
                 mutex_unlock(&bp_mutex);
                 return ret;
             }
         }
         
         /* 测试断点 */
         test_breakpoint_trigger(target_addr);
         mutex_unlock(&bp_mutex);
         return count;
         
     } else if (strcmp(input, "clear") == 0) {
         /* 清除断点 */
         remove_breakpoint();
         target_addr = 0;
         printk(KERN_INFO "Breakpoint cleared\n");
         mutex_unlock(&bp_mutex);
         return count;
         
     } else if (strcmp(input, "status") == 0) {
         /* 显示状态 */
         if (sample_hbp) {
             printk(KERN_INFO "Breakpoint status: Active at 0x%lx\n", target_addr);
         } else if (target_addr != 0) {
             printk(KERN_INFO "Breakpoint status: Inactive (target=0x%lx)\n", target_addr);
         } else {
             printk(KERN_INFO "Breakpoint status: No target set\n");
         }
         mutex_unlock(&bp_mutex);
         return count;
     }
 
     /* 解析地址 */
     ret = kstrtoul(input, 16, &new_addr);
     if (ret) {
         printk(KERN_ERR "Invalid address format: %s\n", input);
         mutex_unlock(&bp_mutex);
         return -EINVAL;
     }
 
     /* 验证地址 */
     ret = validate_target_address(new_addr);
     if (ret) {
         mutex_unlock(&bp_mutex);
         return ret;
     }
 
     /* Remove existing breakpoint */
     remove_breakpoint();
 
     /* Set new target address */
     target_addr = new_addr;
 
     /* Setup new breakpoint */
     ret = setup_breakpoint();
     if (ret) {
         target_addr = 0;  /* 设置失败时清除目标地址 */
         mutex_unlock(&bp_mutex);
         return ret;
     }
 
     mutex_unlock(&bp_mutex);
 
     printk(KERN_INFO "Breakpoint target address set to: 0x%lx\n", target_addr);
     return count;
 }
 
 /* 使用 file_operations 而不是 proc_ops */
 static const struct file_operations bp_proc_fops = {
     .owner = THIS_MODULE,
     .read = bp_proc_read,
     .write = bp_proc_write,
 };
 
 static int __init hw_break_module_init(void)
 {
     /* Check HW breakpoint capability */
     check_hw_breakpoint_capability();
 
     /* Create proc entry */
     proc_entry = proc_create("see", 0644, NULL, &bp_proc_fops);
     if (!proc_entry) {
         printk(KERN_ERR "Failed to create proc entry\n");
         return -ENOMEM;
     }
 
     printk(KERN_INFO "HW Breakpoint module loaded successfully\n");
     printk(KERN_INFO "Proc interface: /proc/see\n");
     printk(KERN_INFO "Commands:\n");
     printk(KERN_INFO "  echo <hex> > /proc/see    # Set breakpoint address\n");
     printk(KERN_INFO "  echo test > /proc/see     # Test breakpoint\n");
     printk(KERN_INFO "  echo clear > /proc/see    # Clear breakpoint\n");
     printk(KERN_INFO "  echo status > /proc/see   # Show status\n");
     printk(KERN_INFO "  cat /proc/see             # Check current status\n");
 
     return 0;
 }
 
 static void __exit hw_break_module_exit(void)
 {
     mutex_lock(&bp_mutex);
     remove_breakpoint();
     mutex_unlock(&bp_mutex);
 
     if (proc_entry)
         remove_proc_entry("see", NULL);
 
     printk(KERN_INFO "HW Breakpoint module unloaded\n");
 }
 
 module_init(hw_break_module_init);
 module_exit(hw_break_module_exit);
 
 MODULE_LICENSE("GPL");
 MODULE_AUTHOR("K.Prasad");
 MODULE_DESCRIPTION("Virtual address breakpoint via proc interface");