KERNELDIR := ../../kernel/kernel_sdk
CURRENT_PATH := $(shell pwd)
obj-m := ap3216c.o

build: kernel_modules
	@cat $(KERNELDIR)/Module.symvers >> Module.symvers

kernel_modules:
	$(MAKE) -C "$(KERNELDIR)" M="$(CURRENT_PATH)" modules
	cp ap3216c.ko /home/he/Desktop/linux/alpha/nfs

clean:
	$(MAKE) -C "$(KERNELDIR)" M="$(CURRENT_PATH)" clean
