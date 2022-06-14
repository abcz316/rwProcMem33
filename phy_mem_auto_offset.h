#ifndef PHY_MEM_AUTO_OFFSET_H_
#define PHY_MEM_AUTO_OFFSET_H_
#include "ver_control.h"

MY_STATIC size_t g_phy_total_memory_size = 0; // 物理内存总大小
MY_STATIC int init_phy_total_memory_size(void) {
	struct sysinfo si;
	unsigned long mem_total, sav_total;
	unsigned int  bitcount = 0;
	unsigned int  mem_unit = 0;
	if (g_phy_total_memory_size) {
		return 0;
	}

	si_meminfo(&si);
	mem_unit = si.mem_unit;

	mem_total = si.totalram;
	while (mem_unit > 1) {
		bitcount++;
		mem_unit >>= 1;
		sav_total = mem_total;
		mem_total <<= 1;
		if (mem_total < sav_total) {
			return 0;
		}
	}
	si.totalram <<= bitcount;
	g_phy_total_memory_size = __pa(si.totalram);
	printk_debug(KERN_INFO "MemTotal si.totalram:%ld\n", si.totalram);
	printk_debug(KERN_INFO "g_phy_total_memory_size:%ld\n", g_phy_total_memory_size);
	return 0;
}

#endif /* PHY_MEM_AUTO_OFFSET_H_ */