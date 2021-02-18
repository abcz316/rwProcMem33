MODULE_NAME := hwBreakpointProc1
RESMAN_CORE_OBJS:=sys.o
RESMAN_GLUE_OBJS:=cvector.o
ifneq ($(KERNELRELEASE),)    
	$(MODULE_NAME)-objs:=$(RESMAN_GLUE_OBJS) $(RESMAN_CORE_OBJS)
	obj-m := hwBreakpointProc1.o
else
	#Ð¡Ã×Max Linux3.10.84
	KDIR := /hydrogen-m-oss_hw/out


all:
	make -C $(KDIR) ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- M=$(PWD) modules
clean:    
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers    
endif    
