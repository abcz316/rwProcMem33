#ifndef TEST_H_
#define TEST_H_
#include "phy_mem.h"
#include "proc_maps.h"
#include "proc_list.h"
#include "proc_cmdline.h"
#include "proc_rss.h"
#include "ver_control.h"

//
//static void test1(void) {
//	size_t phy_addr;
//	struct file * pFile = open_pagemap(14861);
//	printk(KERN_INFO "open_pagemap %d\n", pFile);
//	if (pFile) {
//		phy_addr = get_pagemap_phy_addr(pFile, 0x10106000);
//		printk(KERN_INFO "pagemap phy_addr 0x%llx\n", phy_addr);
//
//		char buf[4];
//		size_t ret;
//		memset(buf, 0, 4);
//		read_ram_physical_addr(true, &ret, phy_addr, buf, 4);
//		if (ret) {
//			int i;
//			for (i = 0; i < 4; i++) {
//				printk(KERN_INFO "[%d]0x%x ", i, buf[i]);
//			}
//		}
//		close_pagemap(pFile);
//	}
//}
//
///*
//static void test2(void)
//{
//	struct pid * proc_pid_struct = get_proc_pid_struct(14861);
//	int map_count = get_proc_map_count(proc_pid_struct);
//	printk(KERN_INFO "map_count:%d\n", map_count);
//
//	char test[8 + 8 + 4 + 4096] = { 0 };
//	get_proc_maps_list(proc_pid_struct, 4096, &test,sizeof(test), true, NULL);
//
//	printk("start:0x%lx,end:0x%lx,flags:%x%x%x%x,name:%s\n",
//		*(unsigned long*)&test[0],
//		*(unsigned long*)&test[8],
//		*(unsigned char*)&test[16],
//		*(unsigned char*)&test[17],
//		*(unsigned char*)&test[18],
//		*(unsigned char*)&test[19],
//		&test[20]);
//
//
//	release_proc_pid_struct(proc_pid_struct);
//
//}
//*/
//
//static void test3(void) {
//	struct pid * proc_pid_struct = get_proc_pid_struct(14861);
//	printk(KERN_INFO "test3 get_proc_pid_struct:%ld\n", proc_pid_struct);
//	if (proc_pid_struct) {
//		size_t phy_addr = 0;
//		pte_t *pte;
//		get_proc_phy_addr(&phy_addr, proc_pid_struct, 0x10106000, &pte);
//		printk(KERN_INFO "calc phy_addr:0x%llx\n", phy_addr);
//
//		release_proc_pid_struct(proc_pid_struct);
//	}
//
//}
//
//static void test4(void) { //TODO cmdline offset. }
//
//static void test5(void) {
//	int *pid = x_kmalloc(sizeof(int) * 100, GFP_KERNEL);
//	int i = 0;
//	int count = get_proc_pid_list(true, (char*)pid, sizeof(int) * 100);
//	printk(KERN_INFO "test5 count:%d\n", count);
//
//	for (i = 0; i < 100; i++) {
//		if (!!pid[i]) {
//			printk(KERN_INFO "test5 pid[%d]:%d\n", i, pid[i]);
//		}
//	}
//
//
//	kfree(pid);
//
//}
//
//static void test6(void) {
//	int ret = 0;
//	struct pid * proc_pid_struct = get_proc_pid_struct(17597);
//
//	printk(KERN_INFO "test6 get_proc_pid_struct:%ld\n", proc_pid_struct);
//
//	ret = set_process_root(proc_pid_struct);
//
//	printk(KERN_INFO "test6 ret:%d\n", ret);
//
//	release_proc_pid_struct(proc_pid_struct);
//
//}
//
//static void test7(void) {
//	struct pid * proc_pid_struct = get_proc_pid_struct(11533);
//	size_t ret = read_proc_rss_size(proc_pid_struct);
//
//	printk(KERN_INFO "test7 get_proc_pid_struct:%ld, ret:%zu\n", proc_pid_struct, ret);
//
//	release_proc_pid_struct(proc_pid_struct);
//
//}
//
//static void test8(void) {
//	struct pid * proc_pid_struct = get_proc_pid_struct(17597);
//	printk(KERN_INFO "test8 get_proc_pid_struct:%ld\n", proc_pid_struct);
//	if (proc_pid_struct) {
//		size_t arg_start = 0, arg_end = 0;
//		int res = get_proc_cmdline_addr(proc_pid_struct, &arg_start, &arg_end);
//		printk(KERN_INFO "test8 get_proc_cmdline_addr arg_start:0x%llx arg_end:0x%llx\n", arg_start, arg_end);
//		release_proc_pid_struct(proc_pid_struct);
//	}
//
//}
//
//static void test9(void) {
//	struct pid * proc_pid_struct = get_proc_pid_struct(14861);
//	printk(KERN_INFO "test9 get_proc_pid_struct:%ld\n", proc_pid_struct);
//	if (proc_pid_struct) {
//
//		//set_process_root(proc_pid_struct);
//
//		release_proc_pid_struct(proc_pid_struct);
//	}
//
//}

#endif /* TEST_H_ */