KERNELDIR := /usr/src/linux-headers-5.4.0-84-generic
CURRENT_PATH := $(shell pwd)
obj-m := chardev.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR)	M=$(CURRENT_PATH) clean