KERNELDIR := ../../kernel/kernel_sdk
CURRENT_PATH := $(shell pwd)

MODULE_NAME := ktrap

obj-m += $(MODULE_NAME).o

$(MODULE_NAME)-objs := \
        ktrap_core.o \
        ktrap_die.o \
        ktrap_oom.o \
        ktrap_panic.o \
        ktrap_reboot.o

DEST_DIR := /home/he/Desktop/linux/alpha/nfs

all: modules install

modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

install:
	@mkdir -p $(DEST_DIR)
	cp $(MODULE_NAME).ko $(DEST_DIR)
	@echo "Artifacts installed to $(DEST_DIR)"

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm -f $(DEST_DIR)/$(MODULE_NAME).ko