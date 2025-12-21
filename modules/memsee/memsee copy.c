/*
 * data_breakpoint.c - Sample HW Breakpoint file to watch kernel data address
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * usage: insmod data_breakpoint.ko phys_addr=<physical_address> [len=<breakpoint_length>]
 *
 * This file is a kernel module that places a breakpoint over physical memory address
 * using Hardware Breakpoint register. The corresponding handler which
 * prints a backtrace is invoked every time a write operation is performed on
 * that memory location.
 *
 * Copyright (C) IBM Corporation, 2009
 *
 * Author: K.Prasad <prasad@linux.vnet.ibm.com>
 * Modified for physical address monitoring
 */
 #include <linux/module.h>	/* Needed by all modules */
 #include <linux/kernel.h>	/* Needed for KERN_INFO */
 #include <linux/init.h>		/* Needed for the macros */
 #include <linux/kallsyms.h>
 #include <linux/io.h>           /* Needed for ioremap */
 
 #include <linux/perf_event.h>
 #include <linux/hw_breakpoint.h>
 
 struct perf_event * __percpu *sample_hbp;
 
 static unsigned long phys_addr = 0X020C406C;
 static int bp_len = HW_BREAKPOINT_LEN_4;
 
 static void __iomem *mapped_addr;
 
 static void sample_hbp_handler(struct perf_event *bp,
                    struct perf_sample_data *data,
                    struct pt_regs *regs)
 {
     printk(KERN_INFO "Physical address 0x%lx value is changed\n", phys_addr);
     dump_stack();
     printk(KERN_INFO "Dump stack from sample_hbp_handler\n");
 }
 
 static int __init hw_break_module_init(void)
 {
     int ret;
     struct perf_event_attr attr;
 
     /* Map physical address to kernel virtual address */
     mapped_addr = ioremap(phys_addr, bp_len);
     if (!mapped_addr) {
         printk(KERN_ERR "Failed to map physical address 0x%lx\n", phys_addr);
         return -ENOMEM;
     }
 
     hw_breakpoint_init(&attr);
     attr.bp_addr = (unsigned long)mapped_addr;
     attr.bp_len = bp_len;
     attr.bp_type = HW_BREAKPOINT_W | HW_BREAKPOINT_R;
 
     sample_hbp = register_wide_hw_breakpoint(&attr, sample_hbp_handler, NULL);
     if (IS_ERR((void __force *)sample_hbp)) {
         ret = PTR_ERR((void __force *)sample_hbp);
         goto fail_unmap;
     }
 
     printk(KERN_INFO "HW Breakpoint for physical address 0x%lx (len=%d) installed\n", 
            phys_addr, bp_len);
 
     return 0;
 
 fail_unmap:
     iounmap(mapped_addr);
     printk(KERN_INFO "Breakpoint registration failed\n");
 
     return ret;
 }
 
 static void __exit hw_break_module_exit(void)
 {
     unregister_wide_hw_breakpoint(sample_hbp);
     if (mapped_addr)
         iounmap(mapped_addr);
     printk(KERN_INFO "HW Breakpoint for physical address 0x%lx uninstalled\n", phys_addr);
 }
 
 module_init(hw_break_module_init);
 module_exit(hw_break_module_exit);
 
 MODULE_LICENSE("GPL");
 MODULE_AUTHOR("K.Prasad");
 MODULE_DESCRIPTION("Physical memory address breakpoint monitor");