#ifndef MY_FILE_OPS_H_
#define MY_FILE_OPS_H_
#include <linux/version.h>
#include <linux/fs.h>
#ifdef __KERNEL__
#include "ver_control.h"
#else
#define MY_CUSTOM_FILE_OPS_MODE
#endif


//这个结构体的大小在各大内核中老变，注意要调整偏移地址
//kernel/include/linux/fs.h

/*

1.检查file_operations结构体的总大小（IDA拖进去后按Shift+F7，找到.rodata字段，搜索文本this_module）
2.检查write_iter与iterate之间有新增函数（Linux内核源码里include/linux/fs.h，file_operations）
3.检查unlocked_ioctl与compat_ioctl的偏移是否对齐
4.检查open与release的偏移是否对齐

*/

#ifdef MY_CUSTOM_FILE_OPS_MODE

#ifndef loff_t
#define loff_t unsigned long
#endif

/*
=========================================================
Linux 5.4
my_file_operations size:0x120(288)
Func:owner, offset:0x00(0)
Func:llseek, offset:0x08(8)
Func:read, offset:0x10(16)
Func:write, offset:0x18(24)
Func:read_iter, offset:0x20(32)
Func:write_iter, offset:0x28(40)
Func:iterate, offset:0x38(56)
Func:iterate_shared, offset:0x40(64)
Func:unlocked_ioctl, offset:0x50(80)
Func:compat_ioctl, offset:0x58(88)
Func:open, offset:0x70(112)
Func:release, offset:0x80(128)
=========================================================

*/
struct my_file_operations {

	struct module *owner; //+0，通常不变
	loff_t(*llseek) (struct file *, unsigned long, int);//+8，通常不变
	ssize_t(*read) (struct file *, char *, size_t, loff_t *); //+16，通常不变
	ssize_t(*write) (struct file *, const char *, size_t, loff_t *); //+24，通常不变
	ssize_t(*read_iter) (void *, void *); //+32，通常不变
	ssize_t(*write_iter) (void *, void *); //+40，通常不变
	int(*iopoll)(void *, int);
	int(*iterate) (void *, void *);
	int(*iterate_shared) (void *, void *);

	unsigned int(*poll) (void *, void *);
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
	long(*ioctl) (void *, unsigned int, unsigned long);
#else
	long(*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
	long(*compat_ioctl) (struct file *, unsigned int, unsigned long);
#endif

	char _1a[8 * 2];

	int(*open) (struct inode *, struct file *);

	char _2[8];

	int(*release) (struct inode *, struct file *);

	char _3[152];
}__attribute__((__packed__));

#else
#define my_file_operations file_operations
#endif // MY_CUSTOM_FILE_OPS_MODE


#endif /* MY_FILE_OPS_H_ */
