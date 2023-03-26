#//
#Makefile
#make -C $(KDIR) M=$(SRCPWD) modules此语句前必须要一个Tab键
#rm -rf chardev.o此语句前必须要一个Tab键
#//

# KDIR:=../../linux-header/linux-4.19.279
KDIR:=/usr/src/linux-headers-5.4.0-84-generic
obj-m:=main.o 
KDIR:=/lib/modules/$(shell uname -r)/build 
#Kernel_Dir
SRCPWD:=$(shell pwd) 
all: 
	make -C $(KDIR) M=$(SRCPWD) modules 
clean: 
	rm -rf main.o
#//
