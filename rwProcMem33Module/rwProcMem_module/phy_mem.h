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

static inline int is_pte_can_read(pte_t* pte);
static inline int is_pte_can_write(pte_t* pte);
static inline int is_pte_can_exec(pte_t* pte);
static inline int change_pte_read_status(pte_t* pte, bool can_read);
static inline int change_pte_write_status(pte_t* pte, bool can_write);
static inline int change_pte_exec_status(pte_t* pte, bool can_exec);

static inline size_t get_proc_phy_addr(struct pid* proc_pid_struct, size_t virt_addr, pte_t** out_pte);
static inline size_t read_ram_physical_addr(bool is_kernel_buf, size_t phy_addr, char* lpBuf, size_t read_size);
static inline size_t write_ram_physical_addr(size_t phy_addr, char* lpBuf, bool is_kernel_buf, size_t write_size);

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

#include <asm/pgtable.h>

static inline size_t get_proc_phy_addr(struct pid* proc_pid_struct, size_t virt_addr, pte_t** out_pte) {
	struct task_struct* task = pid_task(proc_pid_struct, PIDTYPE_PID);
	struct mm_struct *mm = NULL;
	//////////////////////////////////////////////////////////////////////////
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	unsigned long paddr = 0;
	unsigned long page_addr = 0;
	unsigned long page_offset = 0;

	if (!task) { return 0; }

	mm = get_task_mm(task);
	if (!mm) { return 0; }

	*out_pte = 0;
	pgd = x_pgd_offset(mm, virt_addr);
	if (pgd == NULL) {
		printk_debug("pgd is null\n");
		goto out;
	}
	//printk_debug("pgd_val = 0x%lx pgd addr:0x%p\n", (unsigned long int)pgd_val(*pgd), (void*)pgd);
	//printk_debug("init_mm pgd val:0x%lx,pgd addr:0x%p\n", (unsigned long)pgd_val(*(mm->pgd)), (void*)mm->pgd);
	printk_debug("pgd_index = %zu\n", pgd_index(virt_addr));
	if (pgd_none(*pgd)) {
		printk_debug("not mapped in pgd\n");
		goto out;
	}
	printk_debug("pgd_offset ok\n");

	/*
	 * (p4ds are folded into pgds so this doesn't get actually called,
	 * but the define is needed for a generic inline function.)
	 */
	p4d = p4d_offset(pgd, virt_addr);
	//printk_debug("p4d_val = 0x%llx, p4d_index = %d\n", p4d_val(*p4d), p4d_index(virt_addr));
	printk_debug("p4d_val = 0x%llx\n", p4d_val(*p4d));
	if (p4d_none(*p4d))
	{
		printk_debug("not mapped in p4d\n");
		goto out;
	}

	pud = pud_offset(p4d, virt_addr);
	printk_debug("pud_val = 0x%llx \n", pud_val(*pud));
	if (pud_none(*pud)) {
		printk_debug("not mapped in pud\n");
		goto out;
	}
	printk_debug("pud_offset ok\n");

	pmd = pmd_offset(pud, virt_addr);
	printk_debug("pmd_val = 0x%llx\n", pmd_val(*pmd));
	//printk_debug("pmd_index = %d\n", pmd_index(virt_addr));
	if (pmd_none(*pmd)) {
		printk_debug("not mapped in pmd\n");
		goto out;
	}
	printk_debug("pmd_offset ok\n");

	pte = pte_offset_kernel(pmd, virt_addr);
	printk_debug("pte_val = 0x%llx\n", pte_val(*pte));
	//printk_debug("pte_index = %d\n", pte_index(virt_addr));
	if (pte_none(*pte)) {
		printk_debug("not mapped in pte\n");
		goto out;
	}
	printk_debug("pte_offset_kernel ok\n");

	page_addr = page_to_phys(pte_page(*pte));
	page_offset = virt_addr & ~PAGE_MASK;
	paddr = page_addr | page_offset;

	printk_debug("page_addr = %lx, page_offset = %lx\n", page_addr, page_offset);
	printk_debug("vaddr = %zx, paddr = %lx\n", virt_addr, paddr);

	*out_pte = pte;
out:
	mmput(mm);
	return paddr;
}


static inline int is_pte_can_read(pte_t* pte) {
						if (!pte) { return 0; }
#ifdef pte_read
						if (pte_read(*pte)) { return 1; } else { return 0; }
#endif
						return 1;
					}
static inline int is_pte_can_write(pte_t* pte) {
	if (!pte) { return 0; }
	if (pte_write(*pte)) { return 1; } else { return 0; }
}
static inline int is_pte_can_exec(pte_t* pte) {
	if (!pte) { return 0; }
#ifdef pte_exec
	if (pte_exec(*pte)) { return 1; } else { return 0; }
#endif
#ifdef pte_user_exec
	if (pte_user_exec(*pte)) { return 1; } else { return 0; }
#endif
	return 0;
}
static inline int change_pte_read_status(pte_t* pte, bool can_read) {
	if (!pte) { return 0; }
	return 1;
}
static inline int change_pte_write_status(pte_t* pte, bool can_write) {
	if (!pte) { return 0; }
	if (can_write) {
		set_pte(pte, x_pte_mkwrite(*pte));
	} else {
		set_pte(pte, pte_wrprotect(*pte));
	}
	return 1;
}
static inline int change_pte_exec_status(pte_t* pte, bool can_exec) {
	if (!pte) { return 0; }
	if (can_exec) {
#ifdef pte_mknexec
		set_pte(pte, x_pte_mkwrite(*pte));
#endif
	} else {
#ifdef pte_mkexec
		set_pte(pte, x_pte_mkwrite(*pte));
#endif
	}
	return 1;
}

static inline unsigned long size_inside_page(unsigned long start,
	unsigned long size) {
	unsigned long sz;

	sz = PAGE_SIZE - (start & (PAGE_SIZE - 1));

	return min(sz, size);
}


static inline int check_phys_addr_valid_range(size_t addr, size_t count) {
	if (g_phy_total_memory_size == 0) {
		init_phy_total_memory_size();
	}
	return (addr + count) <= g_phy_total_memory_size;
}


static inline size_t read_ram_physical_addr(bool is_kernel_buf, size_t phy_addr, char* lpBuf, size_t read_size) {
	void *bounce;
	size_t realRead = 0;
	if (!check_phys_addr_valid_range(phy_addr, read_size)) {
		printk_debug(KERN_INFO "Error in check_phys_addr_valid_range:%zu,size:%zu\n", phy_addr, read_size);
		return 0;
	}
	bounce = x_kmalloc(PAGE_SIZE, GFP_KERNEL);
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
			printk_debug(KERN_INFO "Error in x_xlate_dev_mem_ptr:0x%zx\n", phy_addr);
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
				printk_debug(KERN_INFO "Error in x_copy_to_user(\n");
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

static inline size_t write_ram_physical_addr(size_t phy_addr, char* lpBuf, bool is_kernel_buf, size_t write_size) {
	size_t realWrite = 0;
	if (!check_phys_addr_valid_range(phy_addr, write_size)) {
		printk_debug(KERN_INFO "Error in check_phys_addr_valid_range:0x%zx,size:%zu\n", phy_addr, write_size);
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
			printk_debug(KERN_INFO "Error in xlate_dev_mem_ptr:0x%zx\n", phy_addr);
			break;
		}
		if (is_kernel_buf) {
			memcpy(ptr, lpBuf, sz);
		} else {
			unsigned long copied = x_copy_from_user(ptr, lpBuf, sz);
			if (copied) {
				unxlate_dev_mem_ptr(phy_addr, ptr);
				realWrite += sz - copied;
				printk_debug(KERN_INFO "Error in x_copy_from_user(\n");
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
#endif /* PHY_MEM_H_ */