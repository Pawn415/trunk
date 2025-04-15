#!/bin/bash
cd rootfs
ls
chmod +x *
find . | cpio -o -H newc | gzip > ../rootfs.cpio.gz
cd -

chmod +x rootfs.cpio.gz
mv rootfs.cpio.gz ../kernel/kernel_sdk/usr/