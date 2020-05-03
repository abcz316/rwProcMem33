#ifndef PHY_MEM_H_
#define PHY_MEM_H_
#include <linux/fs.h>
#include <linux/pid.h>
#include <asm/page.h>


struct file * open_pagemap(int pid);
size_t get_pagemap_phy_addr(struct file * lpPagemap, size_t virt_addr);
void close_pagemap(struct file* lpPagemap);

size_t get_proc_phy_addr(struct pid* proc_pid_struct, size_t virt_addr);

size_t read_ram_physical_addr_to_kernel(size_t phy_addr, char* lpBuf, size_t read_size);
size_t read_ram_physical_addr_to_user(size_t phy_addr, char* lpBuf, size_t read_size);

size_t write_ram_physical_addr_from_kernel(size_t phy_addr, char* lpBuf, size_t write_size);
size_t write_ram_physical_addr_from_user(size_t phy_addr, char* lpBuf, size_t write_size);

static inline unsigned long size_inside_page(unsigned long start,
	unsigned long size)
{
	unsigned long sz;

	sz = PAGE_SIZE - (start & (PAGE_SIZE - 1));

	return min(sz, size);
}
#endif /* PHY_MEM_H_ */