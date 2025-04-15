/*
 * @Author: error: git config user.name && git config user.email & please set dead value or install git
 * @Date: 2022-11-15 23:26:50
 * @LastEditors: HQL
 * @LastEditTime: 2024-02-03 17:41:35
 * @FilePath: \cmd\cmd_update.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <common.h>
#include <command.h>


/*
　　　　第一个参数：添加的命令的名字
　　　　第二个参数：添加的命令最多有几个参数(注意，假如你设置的参数个数是3，而实际的参数个数是4，那么执行命令会输出帮助信息的)
　　　　第三个参数：是否重复(1重复，0不重复)（即按下Enter键的时候，自动执行上次的命令）
　　　　第四个参数：执行函数，即运行了命令具体做啥会在这个函数中体现出来
　　　　第五个参数：帮助信息（short）
　　　　第六个参数：帮助信息（long）
*/
static int do_update(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    /* 判断参数个数 */
    if (argc != 2)
    {
        printf("update params num err\n");
        return 1;
    }

    if (0 == strncmp("uboot", argv[1], sizeof("uboot")))
    {
        printf("update uboot success\n");
    }
    else if (0 == strncmp("image", argv[1], sizeof("image")))
    {
        printf("update image success\n");
    }
    else if (0 == strncmp("rootfs", argv[1], sizeof("rootfs")))
    {
        printf("update rootfs success\n");
    }

    return 0;
}

/* 
U_BOOT_CMD(_name, _maxargs, _rep, _cmd, _usage, _help) 

*/	
U_BOOT_CMD(update, 4, 0, do_update,
	"update command",
	" - check boot progress and timing\n"
	"update all\n"
	"update uboot \n"
	"update image \n"
    "update rootfs \n"
);