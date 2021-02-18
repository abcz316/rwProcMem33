MODULE_NAME := rwProcMem37
RESMAN_CORE_OBJS:=sys.o
RESMAN_GLUE_OBJS:=
ifneq ($(KERNELRELEASE),)    
	$(MODULE_NAME)-objs:=$(RESMAN_GLUE_OBJS) $(RESMAN_CORE_OBJS)
	obj-m := rwProcMem37.o
else
	KDIR := /cepheus-q-oss/out
all:
	make -C $(KDIR) ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- M=$(PWD) modules 
clean:    
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers *.order
endif    
