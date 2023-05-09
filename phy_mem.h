#ifndef PHY_MEM_H_
#define PHY_MEM_H_
//声明
//////////////////////////////////////////////////////////////////////////
#include <linux/fs.h>
#include <linux/pid.h>
#include <asm/page.h>
#include "phy_mem_auto_offset.h"
#include "api_proxy.h"
#include "ver_control.h"

#ifdef CONFIG_USE_PAGEMAP_FILE_CALC_PHY_ADDR
MY_STATIC inline struct file * open_pagemap(int pid);
MY_STATIC size_t get_pagemap_phy_addr(struct file * lpPagemap, size_t virt_addr);
MY_STATIC inline void close_pagemap(struct file* lpPagemap);
#endif

#ifdef CONFIG_USE_PAGE_TABLE_CALC_PHY_ADDR
MY_STATIC inline int is_pte_can_read(pte_t* pte);
MY_STATIC inline int is_pte_can_write(pte_t* pte);
MY_STATIC inline int is_pte_can_exec(pte_t* pte);
MY_STATIC inline int change_pte_read_status(pte_t* pte, bool can_read);
MY_STATIC inline int change_pte_write_status(pte_t* pte, bool can_write);
MY_STATIC inline int change_pte_exec_status(pte_t* pte, bool can_exec);

MY_STATIC inline size_t get_task_proc_phy_addr(struct task_struct* task, size_t virt_addr, pte_t* out_pte);
MY_STATIC inline size_t get_proc_phy_addr(struct pid* proc_pid_struct, size_t virt_addr, pte_t* out_pte);
MY_STATIC inline size_t read_ram_physical_addr(size_t phy_addr, char* lpBuf, bool is_kernel_buf, size_t read_size);
MY_STATIC inline size_t write_ram_physical_addr(size_t phy_addr, char* lpBuf, bool is_kernel_buf, size_t write_size);
#endif


//实现
//////////////////////////////////////////////////////////////////////////
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#if MY_LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,83)
#include <linux/sched/task.h>
#include <linux/sched/mm.h>
#endif


#define RETURN_VALUE(size_t_ptr___out_ret, size_t___value) *size_t_ptr___out_ret=size_t___value;break;


#ifdef CONFIG_USE_PAGEMAP_FILE_CALC_PHY_ADDR
#define PAGEMAP_ENTRY sizeof(uint64_t)
#define GET_BIT(X, Y) (X & ((uint64_t)1 << Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF
const int __endian_bit = 1;
#define is_bigendian() ( (*(char*)&__endian_bit) == 0 )
///////////////////////////////////////////////////////////////////
MY_STATIC inline struct file * open_pagemap(int pid) {
	struct file *filp = NULL;

	char szFilePath[256] = { 0 };
	if (pid == -1) {
		strcpy(szFilePath, "/proc/self/pagemap");
	} else {
		snprintf(szFilePath, sizeof(szFilePath), "/proc/%d/pagemap", pid);
	}

	filp = filp_open(szFilePath, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		return NULL;
	}
	return filp;
}

MY_STATIC size_t get_pagemap_phy_addr(struct file * lpPagemap, size_t virt_addr) {

	uint64_t page_size = PAGE_SIZE;
	uint64_t file_offset;
	mm_segment_t pold_fs;
	uint64_t read_val;
	unsigned char c_buf[PAGEMAP_ENTRY];
	int i;
	unsigned char c;
	printk_debug(KERN_INFO "page_size %zu\n", page_size);
	printk_debug(KERN_INFO "Big endian? %d\n", is_bigendian());

	//Shifting by virt-addr-offset number of bytes
	//and multiplying by the size of an address (the size of an entry in pagemap file)
	file_offset = virt_addr / page_size * PAGEMAP_ENTRY;

	printk_debug(KERN_INFO "Vaddr: 0x%llx, Page_size: %zu, Entry_size: %d\n", virt_addr, page_size, PAGEMAP_ENTRY);

	printk_debug(KERN_INFO "Reading at 0x%llx\n", file_offset);

	pold_fs = get_fs();
	set_fs(KERNEL_DS);
	if (lpPagemap->f_op->llseek(lpPagemap, file_offset, SEEK_SET) == -1) {
		printk_debug(KERN_INFO "Failed to do llseek!");
		set_fs(pold_fs);
		return 0;
	}

	read_val = 0;


	if (lpPagemap->f_op->read(lpPagemap, c_buf, PAGEMAP_ENTRY, &lpPagemap->f_pos) != PAGEMAP_ENTRY) {
		printk_debug(KERN_INFO "Failed to do read!");
		set_fs(pold_fs);
		return 0;
	}
	set_fs(pold_fs);

	if (!is_bigendian()) {
		for (i = 0; i < PAGEMAP_ENTRY / 2; i++) {
			c = c_buf[PAGEMAP_ENTRY - i - 1];
			c_buf[PAGEMAP_ENTRY - i - 1] = c_buf[i];
			c_buf[i] = c;
		}
	}


	for (i = 0; i < PAGEMAP_ENTRY; i++) {
		printk_debug(KERN_INFO "[%d]0x%x ", i, c_buf[i]);

		read_val = (read_val << 8) + c_buf[i];
	}
	printk_debug(KERN_INFO "\n");
	printk_debug(KERN_INFO "Result: 0x%llx\n", read_val);

	if (GET_BIT(read_val, 63)) {
		uint64_t pfn = GET_PFN(read_val);
		printk_debug(KERN_INFO "PFN: 0x%llx (0x%llx)\n", pfn, pfn * page_size + virt_addr % page_size);
		return pfn * page_size + virt_addr % page_size;
	} else {
		printk_debug(KERN_INFO "Page not present\n");
	}
	if (GET_BIT(read_val, 62)) {
		printk_debug(KERN_INFO "Page swapped\n");
	}
	return 0;
}
MY_STATIC inline void close_pagemap(struct file* lpPagemap) {
	filp_close(lpPagemap, NULL);
}
#endif

#ifdef CONFIG_USE_PAGE_TABLE_CALC_PHY_ADDR
#include <asm/pgtable.h>

MY_STATIC inline size_t get_task_proc_phy_addr(struct task_struct* task, size_t virt_addr, pte_t *out_pte) {
	
}


MY_STATIC inline size_t get_proc_phy_addr(struct pid* proc_pid_struct, size_t virt_addr, pte_t* out_pte) {
		struct task_struct* task = pid_task(proc_pid_struct, PIDTYPE_PID);
		if (!task) { return 0; }
		return get_task_proc_phy_addr(task, virt_addr, out_pte);
}


MY_STATIC inline int is_pte_can_read(pte_t* pte) {
						if (!pte) { return 0; }
#ifdef pte_read
						if (pte_read(*pte)) { return 1; } else { return 0; }
#endif
						return 1;
					}
MY_STATIC inline int is_pte_can_write(pte_t* pte) {
	if (!pte) { return 0; }
	if (pte_write(*pte)) { return 1; } else { return 0; }
}
MY_STATIC inline int is_pte_can_exec(pte_t* pte) {
	if (!pte) { return 0; }
#ifdef pte_exec
	if (pte_exec(*pte)) { return 1; } else { return 0; }
#endif
#ifdef pte_user_exec
	if (pte_user_exec(*pte)) { return 1; } else { return 0; }
#endif
	return 0;
}
MY_STATIC inline int change_pte_read_status(pte_t* pte, bool can_read) {
	//ARM64架构所有内存都具备可读属性
	if (!pte) { return 0; }
	return 1;
}
MY_STATIC inline int change_pte_write_status(pte_t* pte, bool can_write) {
	//ARM64架构所有内存都必须具备可读属性
	if (!pte) { return 0; }
	if (can_write) {
		//ARM64：删除内存“仅可读”属性，此时内存内存可读、可写
		set_pte(pte, pte_mkwrite(*pte));
	} else {
		//ARM64：设置内存“仅可读”属性，此时内存内存可读、不可写
		set_pte(pte, pte_wrprotect(*pte));
	}
	return 1;
}
MY_STATIC inline int change_pte_exec_status(pte_t* pte, bool can_exec) {
	if (!pte) { return 0; }
	if (can_exec) {
#ifdef pte_mknexec
		set_pte(pte, pte_mknexec(*pte));
#endif
	} else {
#ifdef pte_mkexec
		set_pte(pte, pte_mkexec(*pte));
#endif
	}
	return 1;
}



#endif



MY_STATIC inline unsigned long size_inside_page(unsigned long start,
	unsigned long size) {
	unsigned long sz;

	sz = PAGE_SIZE - (start & (PAGE_SIZE - 1));

	return min(sz, size);
}


MY_STATIC inline int check_phys_addr_valid_range(size_t addr, size_t count) {
	if (g_phy_total_memory_size == 0) {
		init_phy_total_memory_size();
	}
	return (addr + count) <= g_phy_total_memory_size;
}


MY_STATIC inline size_t read_ram_physical_addr(size_t phy_addr, char* lpBuf, bool is_kernel_buf, size_t read_size) {
	void *bounce;
	size_t realRead = 0;
	if (!check_phys_addr_valid_range(phy_addr, read_size)) {
		printk_debug(KERN_INFO "Error in check_phys_addr_valid_range:%p,size:%zu\n", phy_addr, read_size);
		return 0;
	}
	bounce = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!bounce) {
		return 0;
	}

	while (read_size > 0) {
		size_t sz = size_inside_page(phy_addr, read_size);

		/*
		 * On ia64 if a page has been mapped somewhere as uncached, then
		 * it must also be accessed uncached by the kernel or data
		 * corruption may occur.
		 */

		char *ptr = xlate_dev_mem_ptr(phy_addr);
		int probe;

		if (!ptr) {
			printk_debug(KERN_INFO "Error in x_xlate_dev_mem_ptr:0x%llx\n", phy_addr);
			break;
		}
		probe = x_probe_kernel_read(bounce, ptr, sz);
		unxlate_dev_mem_ptr(phy_addr, ptr);
		if (probe) {
			break;
		}
		if (is_kernel_buf) {
			memcpy(lpBuf, bounce, sz);
		} else {
			unsigned long remaining = x_copy_to_user(lpBuf, bounce, sz);
			if (remaining) {
				printk_debug(KERN_INFO "Error in x_copy_to_user\n");
				break;
			}
		}
		lpBuf += sz;
		phy_addr += sz;
		read_size -= sz;
		realRead += sz;
	}
	kfree(bounce);
	return realRead;
}

MY_STATIC inline size_t write_ram_physical_addr(size_t phy_addr, char* lpBuf, bool is_kernel_buf, size_t write_size) {
	size_t realWrite = 0;
	if (!check_phys_addr_valid_range(phy_addr, write_size)) {
		printk_debug(KERN_INFO "Error in check_phys_addr_valid_range:0x%llx,size:%zu\n", phy_addr, write_size);
		return 0;
	}


	while (write_size > 0) {
		size_t sz = size_inside_page(phy_addr, write_size);

		/*
			* On ia64 if a page has been mapped somewhere as uncached, then
			* it must also be accessed uncached by the kernel or data
			* corruption may occur.
			*/

		char *ptr = xlate_dev_mem_ptr(phy_addr);
		if (!ptr) {
			printk_debug(KERN_INFO "Error in xlate_dev_mem_ptr:0x%llx\n", phy_addr);
			break;
		}
		if (is_kernel_buf) {
			memcpy(ptr, lpBuf, sz);
		} else {
			unsigned long copied = x_copy_from_user(ptr, lpBuf, sz);
			if (copied) {
				unxlate_dev_mem_ptr(phy_addr, ptr);
				realWrite += sz - copied;
				printk_debug(KERN_INFO "Error in x_copy_from_user\n");
				break;
			}
		}
		unxlate_dev_mem_ptr(phy_addr, ptr);

		lpBuf += sz;
		phy_addr += sz;
		write_size -= sz;
		realWrite += sz;
	}
	return realWrite;
}



//Update - RW party: drivers/char/mem.c

#endif /* PHY_MEM_H_ */