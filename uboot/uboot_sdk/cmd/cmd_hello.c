// #include <command.h>
// #include <console.h>
// #include <linux/compiler.h>

// extern char __hik_boot_start[];
// extern char __hik_boot_end[];

// static int do_show_boot_data(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
// {
//     ulong start_addr = (ulong)__hik_boot_start;
//     ulong end_addr = (ulong)__hik_boot_end;
//     ulong length = end_addr - start_addr;

//     /* 验证地址有效性 */
//     if (start_addr >= end_addr) {
//         printf("Error: Invalid address range!\n");
//         return CMD_RET_FAILURE;
//     }

//     printf("HIK Boot Data (%lu bytes):\n", length);
//     print_buffer(start_addr, (void *)start_addr, 1, length, 16);

//     return CMD_RET_SUCCESS;
// }

// U_BOOT_CMD(
//     show_bootdata, 1, 1, do_show_boot_data,
//     "Display HIK boot data section",
//     "\nPrints the content of the .hqldata* section"
// );


#include <common.h>
#include <command.h>

/* 调试打印宏，自动输出函数名和行号 */
#ifndef DBG_PRINT
#define DBG_PRINT(fmt, ...) \
    printf("%s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#endif

extern char __hik_boot_start[];
extern char __hik_boot_end[];

/* 自定义命令处理函数 */
static int do_hello(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    void (*entry)(void) = (void (*)(void))__hik_boot_start;
    ulong rc;
    
    /* 验证地址对齐 */
    if ((ulong)entry % 4 != 0) {
        printf("Error: Binary not 4-byte aligned (0x%08lx)\n", (ulong)entry);
        return CMD_RET_FAILURE;
    }
    
    /* 打印调试信息 */
    size_t bin_size = (ulong)__hik_boot_end - (ulong)__hik_boot_start;
    printf("## Running embedded binary [0x%08lx - 0x%08lx, size: %lu bytes]\n",
           (ulong)__hik_boot_start, (ulong)__hik_boot_end, bin_size);
    
    /* 跳转执行 */
    printf("## Starting binary at 0x%08lx ...\n", (ulong)entry);
    rc = ((ulong (*)(void))entry)();
    
    printf("## Binary returned: 0x%08lx\n", rc);
    return (rc != 0) ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

/* 把命令注册到 U-Boot */
U_BOOT_CMD(
    hello,               /* 命令名 */
    1,                     /* 最大参数数目 */
    0,                     /* 直接重入(1)或不可重入(0) */
    do_hello,            /* 处理函数 */
    "do_hello", /* 简短描述 */
    "hello test" /* 用法说明 */
);
