/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <exports.h>

int hello_world (int argc, char * const argv[])
{
	int i;

	/* Print the ABI version */
	app_startup(argv);
	printf ("Example expects ABI 22 version %d\n", XF_VERSION);
	printf ("Actual U-Boot ABI version %d\n", (int)get_version());

	printf ("Hello World 44\n");

	printf ("argc = %d\n", argc);

	printf ("Hit ctrl+u to exit ... ");
    while (1) {
            int c = getc();
            printf(" [Debug: Received 0x%02X]\n", c);  // 调试输出
            
            if (c == 0x15) {   // 检查是否是 Ctrl+U
                break;
            }
	}
	printf ("\n\n");
	return (0);
}
