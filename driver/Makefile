DEVICE_NAME := colenda_driver

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
INCLUDE_DIR := $(PWD)/include
SRC_DIR := $(PWD)/src

SRC_FILES := $(SRC_DIR)/colenda_driver.c
INCLUDE_FILES := $(INCLUDE_DIR)/arm_address_map.h

obj-m := $(DEVICE_NAME).o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install: all
	sudo cp $(DEVICE_NAME).ko /lib/modules/$(shell uname -r)/kernel/drivers/misc/
	sudo depmod -a
	sudo modprobe $(DEVICE_NAME)
	dmesg | tail -n 1

uninstall:
	rm -f $(DEVICE_NAME).mod.c $(DEVICE_NAME).mod.o $(DEVICE_NAME).o $(DEVICE_NAME).ko Module.symvers modules.order
	sudo modprobe -r $(DEVICE_NAME)
	sudo rm -f /lib/modules/$(shell uname -r)/kernel/drivers/misc/$(DEVICE_NAME).ko
	sudo depmod -a
	sudo rm -f /dev/$(DEVICE_NAME)