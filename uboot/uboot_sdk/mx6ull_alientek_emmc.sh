#!/bin/bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- distclean
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- mx6ull_alientek_emmc_defconfig
make V=0  ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j16


cp -f u-boot.imx /home/alientek/linux/tftp

echo "u-boot build ok!!!"