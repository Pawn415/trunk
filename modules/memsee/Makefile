KERNELDIR := ../../kernel/kernel_sdk
CURRENT_PATH := $(shell pwd)

# Names and filenames for both modules
MODULE1_NAME := see
MODULE2_NAME := memw

obj-m := $(MODULE1_NAME).o $(MODULE2_NAME).o

MODULE1_KO := $(MODULE1_NAME).ko
MODULE2_KO := $(MODULE2_NAME).ko

# Destination for artifacts
DEST_DIR := /home/he/Desktop/linux/alpha/nfs

# Default target: build both modules and install
all: kernel_modules install

# Build kernel modules
kernel_modules:
	$(MAKE) -C "$(KERNELDIR)" M="$(CURRENT_PATH)" modules

# Install artifacts to DEST_DIR
install: kernel_modules
	@mkdir -p $(DEST_DIR)
	cp $(MODULE1_KO) $(MODULE2_KO) $(DEST_DIR)
	@echo "Artifacts installed to $(DEST_DIR)"

# Clean both modules
clean:
	$(MAKE) -C "$(KERNELDIR)" M="$(CURRENT_PATH)" clean
	rm -f $(MODULE1_KO) $(MODULE2_KO)