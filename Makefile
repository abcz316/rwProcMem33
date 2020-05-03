MODULE_NAME := rwProcMem33
RESMAN_CORE_OBJS:=sys.o
RESMAN_GLUE_OBJS:=phy_mem.o remote_proc_maps.o  remote_proc_cmdline.o remote_proc_rss.o
ifneq ($(KERNELRELEASE),)    
	$(MODULE_NAME)-objs:=$(RESMAN_GLUE_OBJS) $(RESMAN_CORE_OBJS)
	obj-m := rwProcMem33.o
else
	KDIR := /hydrogen-m-oss/out	
all:
	make -C $(KDIR) ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- M=$(PWD) modules 
clean:    
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers *.order
endif    
