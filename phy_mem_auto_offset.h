#ifndef PHY_MEM_AUTO_OFFSET_H_
#define PHY_MEM_AUTO_OFFSET_H_
#include "api_proxy.h"
#include "ver_control.h"

#undef pgd_offset
#if MY_LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,84)
//来源：#define pgd_offset(mm, addr)	((mm)->pgd+pgd_index(addr))
#define my_pgd_offset(pgd, addr)	(pgd+pgd_index(addr))
#define my_pud_offset(dir, addr) ((pud_t *)__va(pud_offset_phys((dir), (addr))))
#endif
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(5,10,43)
//来源：#define pgd_offset(mm, addr)	((mm)->pgd+pgd_index(addr))
#define my_pgd_offset(pgd, addr)	(pgd+pgd_index(addr))
#define my_pud_offset(dir, addr) ((pud_t *)__va(pud_offset_phys((dir), (addr))))
#endif
#if MY_LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,43)
//来源：#define pgd_offset(mm, address)	pgd_offset_pgd((mm)->pgd, (address))
#define my_pgd_offset(pgd, address)	pgd_offset_pgd(pgd, address)
#endif

#define my_get_fs()	(current_thread_info()->addr_limit)

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

#ifdef CONFIG_USE_PAGE_TABLE_CALC_PHY_ADDR
MY_STATIC ssize_t g_pgd_offset_mm_struct = 0;
MY_STATIC bool g_init_pgd_offset_success = false;
MY_STATIC int init_pgd_offset(struct mm_struct *mm) {
	int is_find_pgd_offset = 0;

	g_init_pgd_offset_success = true;
	for (g_pgd_offset_mm_struct = -40; g_pgd_offset_mm_struct <= 80; g_pgd_offset_mm_struct += 1) {
		//精确偏移
		char *rp;
		size_t val;
		ssize_t accurate_offset = (ssize_t)((size_t)&mm->pgd - (size_t)mm + g_pgd_offset_mm_struct);
		if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t)) {
			return -EFAULT;
		}
		rp = (char*)((size_t)mm + (size_t)accurate_offset);
		val = *(size_t*)(rp);
		printk_debug(KERN_EMERG "init_pgd_offset %zd:%zd:%p:%ld\n", g_pgd_offset_mm_struct, accurate_offset, rp, val);

		if (val == TASK_SIZE) {
			//找到了
			g_pgd_offset_mm_struct += sizeof(void*) * 2; //再跳过两个变量
			printk_debug(KERN_EMERG "found g_init_pgd_offset_success:%zd\n", g_pgd_offset_mm_struct);
			is_find_pgd_offset = 1;
			break;
		}
	}


	if (!is_find_pgd_offset) {
		g_init_pgd_offset_success = false;
		printk_debug(KERN_INFO "find pgd offset failed\n");
		return -ESPIPE;
	}
	printk_debug(KERN_INFO "g_pgd_offset_mm_struct:%zu\n", g_pgd_offset_mm_struct);
	return 0;
}

MY_STATIC inline pgd_t *x_pgd_offset(struct mm_struct *mm, size_t addr) {
	size_t pgd;
	ssize_t accurate_offset;
	if (g_init_pgd_offset_success == false) {
		if (init_pgd_offset(mm) != 0) {
			return NULL;
		}
	}
	//精确偏移
	accurate_offset = (ssize_t)((size_t)&mm->pgd - (size_t)mm + g_pgd_offset_mm_struct);
	printk_debug(KERN_INFO "x_pgd_offset accurate_offset:%zd\n", accurate_offset);
	if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t)) {
		return NULL;
	}

	//拷贝到我自己的pgd指针变量里去
	//写法一（可读性强）
	//void * rv = (size_t*)((size_t)mm + (size_t)accurate_offset);
	//pgd_t *pgd;
	//memcpy(&pgd, rv, sizeof(pgd_t *));

	//写法二（快些）
	pgd = *(size_t*)((size_t)mm + (size_t)accurate_offset);

	return my_pgd_offset((pgd_t*)pgd, addr);
}
#endif

#endif /* PHY_MEM_AUTO_OFFSET_H_ */