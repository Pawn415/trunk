KERNELDIR := ../../kernel/kernel_sdk
CURRENT_PATH := $(shell pwd)
obj-m := ramdisk.o

build: kernel_modules
	@cat $(KERNELDIR)/Module.symvers >> Module.symvers

kernel_modules:
	$(MAKE) -C "$(KERNELDIR)" M="$(CURRENT_PATH)" modules
	cp ramdisk.ko /home/he/Desktop/linux/alpha/nfs

clean:
	$(MAKE) -C "$(KERNELDIR)" M="$(CURRENT_PATH)" clean
