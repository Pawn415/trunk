KERNELDIR := /home/he/Desktop/linux/alpha/trunk/kernel_sdk
CURRENT_PATH := $(shell pwd)

obj-m := virt_dev.o
EXTRA_CFLAGS +=-Wno-error=date-time
EXTRA_CFLAGS +=-Wno-date-time
build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	cp -f virt_dev.ko ../../nfs

clean:
	rm -rf virt_App
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
