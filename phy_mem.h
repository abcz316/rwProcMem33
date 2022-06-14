#ifndef PHY_MEM_H_
#define PHY_MEM_H_
//声明
//////////////////////////////////////////////////////////////////////////
#include <linux/fs.h>
#include <linux/pid.h>
#include <asm/page.h>
#include "phy_mem_auto_offset.h"
#include "ver_control.h"

#ifdef CONFIG_USE_PAGEMAP_FILE
MY_STATIC inline struct file * open_pagemap(int pid);
MY_STATIC size_t get_pagemap_phy_addr(struct file * lpPagemap, size_t virt_addr);
MY_STATIC inline void close_pagemap(struct file* lpPagemap);
#else
MY_STATIC inline int is_pte_can_read(pte_t* pte);
MY_STATIC inline int is_pte_can_write(pte_t* pte);
MY_STATIC inline int is_pte_can_exec(pte_t* pte);
MY_STATIC inline int change_pte_read_status(pte_t* pte, bool can_read);
MY_STATIC inline int change_pte_write_status(pte_t* pte, bool can_write);
MY_STATIC inline int change_pte_exec_status(pte_t* pte, bool can_exec);

//size_t get_task_proc_phy_addr(struct task_struct* task, size_t virt_addr, pte_t *out_pte)
//size_t get_proc_phy_addr(struct pid* proc_pid_struct, size_t virt_addr, pte_t *out_pte)
//size_t read_ram_physical_addr(size_t phy_addr, char* lpBuf, bool is_kernel_buf, size_t read_size)
//size_t write_ram_physical_addr(size_t phy_addr, char* lpBuf, bool is_kernel_buf, size_t write_size)
#endif



//实现
//////////////////////////////////////////////////////////////////////////
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/highmem.h>
#include <linux/slab.h>


#define RETURN_VALUE(size_t_ptr___out_ret, size_t___value) *size_t_ptr___out_ret=size_t___value;break;


#ifdef CONFIG_USE_PAGEMAP_FILE
#define PAGEMAP_ENTRY sizeof(uint64_t)
#define GET_BIT(X, Y) (X & ((uint64_t)1 << Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF
const int __endian_bit = 1;
#define is_bigendian() ( (*(char*)&__endian_bit) == 0 )
///////////////////////////////////////////////////////////////////
MY_STATIC inline struct file * open_pagemap(int pid)
{
	struct file *filp = NULL;

	char szFilePath[256] = { 0 };
	if (pid == -1)
	{
		strcpy(szFilePath,"/proc/self/pagemap");
	}
	else
	{
		snprintf(szFilePath, sizeof(szFilePath), "/proc/%d/pagemap", pid);
	}
	
	filp = filp_open(szFilePath, O_RDONLY, 0);
	if (IS_ERR(filp))
	{
		return NULL;
	}
	return filp;
}

MY_STATIC size_t get_pagemap_phy_addr(struct file * lpPagemap, size_t virt_addr)
{

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
	if (lpPagemap->f_op->llseek(lpPagemap, file_offset, SEEK_SET) == -1)
	{
		printk_debug(KERN_INFO "Failed to do llseek!");
		set_fs(pold_fs);
		return 0;
	}

	read_val = 0;


	if (lpPagemap->f_op->read(lpPagemap, c_buf, PAGEMAP_ENTRY, &lpPagemap->f_pos) != PAGEMAP_ENTRY)
	{
		printk_debug(KERN_INFO "Failed to do read!");
		set_fs(pold_fs);
		return 0;
	}
	set_fs(pold_fs);

	if (!is_bigendian())
	{
		for (i = 0; i < PAGEMAP_ENTRY / 2; i++)
		{
			c = c_buf[PAGEMAP_ENTRY - i - 1];
			c_buf[PAGEMAP_ENTRY - i - 1] = c_buf[i];
			c_buf[i] = c;
		}
	}


	for (i = 0; i < PAGEMAP_ENTRY; i++)
	{
		printk_debug(KERN_INFO "[%d]0x%x ", i, c_buf[i]);

		read_val = (read_val << 8) + c_buf[i];
	}
	printk_debug(KERN_INFO "\n");
	printk_debug(KERN_INFO "Result: 0x%llx\n", read_val);

	if (GET_BIT(read_val, 63))
	{
		uint64_t pfn = GET_PFN(read_val);
		printk_debug(KERN_INFO "PFN: 0x%llx (0x%llx)\n", pfn, pfn * page_size + virt_addr % page_size);
		return pfn * page_size + virt_addr % page_size;
	}
	else
	{
		printk_debug(KERN_INFO "Page not present\n");
	}
	if (GET_BIT(read_val, 62))
	{
		printk_debug(KERN_INFO "Page swapped\n");
	}
	return 0;
}
MY_STATIC inline void close_pagemap(struct file* lpPagemap)
{
	filp_close(lpPagemap, NULL);
}
#else
#include <asm/pgtable.h>
MY_STATIC inline int is_pte_can_read(pte_t* pte)
{
	if (!pte) { return 0; }
#ifdef pte_read
	if (pte_read(*pte)) { return 1; }
	else { return 0; }
#endif
	return 1;
}
MY_STATIC inline int is_pte_can_write(pte_t* pte)
{
	if (!pte) { return 0; }
	if (pte_write(*pte)) { return 1; }
	else { return 0; }
}
MY_STATIC inline int is_pte_can_exec(pte_t* pte)
{
	if (!pte) { return 0; }
#ifdef pte_exec
	if (pte_exec(*pte)) { return 1; }
	else { return 0; }
#endif
#ifdef pte_user_exec
	if (pte_user_exec(*pte)) { return 1; }
	else { return 0; }
#endif
	return 0;
}
MY_STATIC inline int change_pte_read_status(pte_t* pte, bool can_read)
{
	//ARM64架构所有内存都具备可读属性
	if (!pte) { return 0; }
	return 1;
}
MY_STATIC inline int change_pte_write_status(pte_t* pte, bool can_write)
{
	//ARM64架构所有内存都必须具备可读属性
	if (!pte) { return 0; }
	if (can_write)
	{
		//ARM64：删除内存“仅可读”属性，此时内存内存可读、可写
		set_pte(pte, pte_mkwrite(*pte));
	}
	else
	{
		//ARM64：设置内存“仅可读”属性，此时内存内存可读、不可写
		set_pte(pte, pte_wrprotect(*pte));
	}
	return 1;
}
MY_STATIC inline int change_pte_exec_status(pte_t* pte, bool can_exec)
{
	if (!pte) { return 0; }
	if (can_exec)
	{
#ifdef pte_mknexec
		set_pte(pte, pte_mknexec(*pte));
#endif
	}
	else
	{
#ifdef pte_mkexec
		set_pte(pte, pte_mkexec(*pte));
#endif
	}
	return 1;
}
//
//MY_STATIC size_t get_task_proc_phy_addr(struct task_struct* task, size_t virt_addr, pte_t *out_pte)
//{
	/*Because this code is only for the purpose of learning and research, it is forbidden to use this code to do bad things, so I only release the method code to obtain the physical memory address through the pagemap file here, and the method to calculate the physical memory address can be realized without relying on pagemap and pure algorithm, and I have implemented it, but in order to prevent some people from doing bad things, this part of the code I'm not open. If you need this part of the code, you can contact me and ask me for this part of the code. Of course, you can also add the relevant algorithm code here by yourself. Here I can provide a brief process. You can browse the relevant source code of pagemap in Linux kernel, and calculate the address of physical memory by mixing the PGD, PUD, PMD, PTE and page of the process .*/
//	return 0;
//}

#define get_task_proc_phy_addr(size_t_ptr___out_ret, task_struct_ptr___task, size_t___virt_addr, pte_t_ptr__out_pte) \
do{\
	/*Because this code is only for the purpose of learning and research, it is forbidden to use this code to do bad things, so I only release the method code to obtain the physical memory address through the pagemap file here, and the method to calculate the physical memory address can be realized without relying on pagemap and pure algorithm, and I have implemented it, but in order to prevent some people from doing bad things, this part of the code I'm not open. If you need this part of the code, you can contact me and ask me for this part of the code. Of course, you can also add the relevant algorithm code here by yourself. Here I can provide a brief process. You can browse the relevant source code of pagemap in Linux kernel, and calculate the address of physical memory by mixing the PGD, PUD, PMD, PTE and page of the process .*/\
	size_t * ret___ = size_t_ptr___out_ret;\
	struct task_struct* task___ = task_struct_ptr___task;\
	RETURN_VALUE(ret___, 0)\
}while(0)



//MY_STATIC size_t get_proc_phy_addr(struct pid* proc_pid_struct, size_t virt_addr, pte_t *out_pte)
//{
//	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
//	if (!task) { return 0; }
//	return get_task_proc_phy_addr(task, virt_addr, out_pte);
//}
#define get_proc_phy_addr(size_t_ptr___out_ret, pid_ptr___proc_pid_struct, size_t___virt_addr, pte_t_ptr__out_pte) \
do{\
	struct task_struct *task_try___ = get_pid_task(pid_ptr___proc_pid_struct, PIDTYPE_PID);\
	if (!task_try___) { 	RETURN_VALUE(size_t_ptr___out_ret, 0) }\
	\
	get_task_proc_phy_addr(size_t_ptr___out_ret,task_try___, size_t___virt_addr, pte_t_ptr__out_pte);\
}while(0)


#endif




MY_STATIC inline unsigned long size_inside_page(unsigned long start,
	unsigned long size)
{
	unsigned long sz;

	sz = PAGE_SIZE - (start & (PAGE_SIZE - 1));

	return min(sz, size);
}


MY_STATIC inline int check_phys_addr_valid_range(size_t addr, size_t count) {
	if (g_phy_total_memory_size == 0) {
		init_phy_total_memory_size();
	}
	return addr + count <= g_phy_total_memory_size;
}




#ifndef xlate_dev_mem_ptr
#define xlate_dev_mem_ptr(p) __va(p)
#endif

#ifndef unxlate_dev_mem_ptr
#define unxlate_dev_mem_ptr(phys, addr)
#endif

//MY_STATIC size_t read_ram_physical_addr(size_t phy_addr, char* lpBuf, bool is_kernel_buf, size_t read_size)
//{
//	void *bounce;
//	size_t realRead = 0;
//	//检查物理地址是否合法
//	if (!check_phys_addr_valid_range(phy_addr, read_size))
//	{
//		printk_debug(KERN_INFO "Error in check_phys_addr_valid_range:0x%llx,size:%zu\n", phy_addr, read_size);
//		return 0;
//	}
//	bounce = kmalloc(PAGE_SIZE, GFP_KERNEL);
//	if (!bounce)
//	{
//		return 0;
//	}
//
//	while (read_size > 0)
//	{
//		size_t sz = size_inside_page(phy_addr, read_size);
//
//		/*
//		 * On ia64 if a page has been mapped somewhere as uncached, then
//		 * it must also be accessed uncached by the kernel or data
//		 * corruption may occur.
//		 */
//
//		char *ptr = xlate_dev_mem_ptr(phy_addr);
//		int probe;
//
//		if (!ptr)
//		{
//			printk_debug(KERN_INFO "Error in xlate_dev_mem_ptr:0x%llx\n", phy_addr);
//			break;
//		}
//		probe = x_probe_kernel_read(bounce, ptr, sz);
//		unxlate_dev_mem_ptr(phy_addr, ptr);
//		if (probe)
//		{
//			break;
//		}
//		if (is_kernel_buf)
//		{
//			memcpy(lpBuf, bounce, sz);
//		}
//		else
//		{
//			unsigned long remaining = copy_to_user(lpBuf, bounce, sz);
//			if (remaining)
//			{
//				printk_debug(KERN_INFO "Error in copy_to_user\n");
//				break;
//			}
//		}
//		lpBuf += sz;
//		phy_addr += sz;
//		read_size -= sz;
//		realRead += sz;
//	}
//	kfree(bounce);
//	return realRead;
//}
#define read_ram_physical_addr(size_t_ptr___out_ret, size_t___phy_addr, char_ptr___lpBuf, bool___is_kernel_buf, size_t___read_size)\
do{\
	size_t *ret___ = size_t_ptr___out_ret;\
	size_t phy_addr___ = size_t___phy_addr;\
	char* lpBuf___ = char_ptr___lpBuf;\
	bool is_kernel_buf___ = bool___is_kernel_buf;\
	size_t read_size___ = size_t___read_size;\
\
	void *bounce___;\
	size_t realRead___ = 0;\
\
	/*检查物理地址是否合法*/\
	if (!check_phys_addr_valid_range(phy_addr___, read_size___))\
	{\
		printk_debug(KERN_INFO "Error in check_phys_addr_valid_range:0x%zx,size:%zu\n", phy_addr___, read_size___);\
		RETURN_VALUE(ret___, realRead___) \
	}\
	bounce___ = kmalloc(PAGE_SIZE, GFP_KERNEL);\
	if (!bounce___) { RETURN_VALUE(ret___, realRead___) } \
\
	while (read_size___ > 0)\
	{\
		size_t sz___ = size_inside_page(phy_addr___, read_size___);\
\
		/*
		 * On ia64 if a page has been mapped somewhere as uncached, then
		 * it must also be accessed uncached by the kernel or data
		 * corruption may occur.
		 */\
\
		char *ptr___ = xlate_dev_mem_ptr(phy_addr___);\
		int probe___;\
\
		if (!ptr___)\
		{\
			printk_debug(KERN_INFO "Error in xlate_dev_mem_ptr:0x%zx\n", phy_addr___);\
			RETURN_VALUE(ret___, realRead___) \
		}\
		probe___ = x_probe_kernel_read(bounce___, ptr___, sz___);\
		unxlate_dev_mem_ptr(phy_addr___, ptr___);\
		if (probe___)\
		{\
			RETURN_VALUE(ret___, realRead___) \
		}\
\
		if (is_kernel_buf___)\
		{\
			memcpy(lpBuf___, bounce___, sz___); \
		}\
		else\
		{\
			unsigned long remaining___ = copy_to_user(lpBuf___, bounce___, sz___); \
			if (remaining___)\
			{\
				printk_debug(KERN_INFO "Error in copy_to_user\n"); \
				RETURN_VALUE(ret___, realRead___) \
			}\
		}\
\
\
		lpBuf___ += sz___;\
		phy_addr___ += sz___;\
		read_size___ -= sz___;\
		realRead___ += sz___;\
	}\
	kfree(bounce___);\
	RETURN_VALUE(ret___, realRead___) \
}while(0)




//MY_STATIC size_t write_ram_physical_addr(size_t phy_addr, char* lpBuf, bool is_kernel_buf, size_t write_size)
//{
//	size_t realWrite = 0;
//	//检查物理地址是否合法
//	if (!check_phys_addr_valid_range(phy_addr, write_size))
//	{
//		printk_debug(KERN_INFO "Error in check_phys_addr_valid_range:0x%llx,size:%zu\n", phy_addr, write_size);
//		return 0;
//	}
//
//
//	while (write_size > 0)
//	{
//		size_t sz = size_inside_page(phy_addr, write_size);
//
//		/*
//		 * On ia64 if a page has been mapped somewhere as uncached, then
//		 * it must also be accessed uncached by the kernel or data
//		 * corruption may occur.
//		 */
//
//		char *ptr = xlate_dev_mem_ptr(phy_addr);
//		unsigned long copied;
//		if (!ptr)
//		{
//			printk_debug(KERN_INFO "Error in xlate_dev_mem_ptr:0x%llx\n", phy_addr);
//			break;
//		}
//		if (is_kernel_buf)
//		{
//			memcpy(ptr, lpBuf, sz);
//		}
//		else
//		{
//			unsigned long copied = copy_from_user(ptr, lpBuf, sz);
//			if (copied)
//			{
//				unxlate_dev_mem_ptr(phy_addr, ptr);
//				realWrite += sz - copied;
//				printk_debug(KERN_INFO "Error in copy_from_user\n");
//				break;
//			}
//		}
//		unxlate_dev_mem_ptr(phy_addr, ptr);
//
//		lpBuf += sz;
//		phy_addr += sz;
//		write_size -= sz;
//		realWrite += sz;
//	}
//	return realWrite;
//}

#define write_ram_physical_addr(size_t_ptr___out_ret, size_t___phy_addr, char_ptr___lpBuf, bool___is_kernel_buf, size_t___write_size)\
do {\
	size_t *ret___ = size_t_ptr___out_ret; \
	size_t phy_addr___ = size_t___phy_addr; \
	char* lpBuf___ = char_ptr___lpBuf; \
	bool is_kernel_buf___ = bool___is_kernel_buf; \
	size_t write_size___ = size_t___write_size; \
\
	size_t realWrite___ = 0;\
	/*检查物理地址是否合法*/\
	if (!check_phys_addr_valid_range(phy_addr___, write_size___))\
	{\
		printk_debug(KERN_INFO "Error in check_phys_addr_valid_range:0x%zx,size:%zu\n", phy_addr___, write_size___);\
		RETURN_VALUE(ret___,realWrite___) \
	}\
	while (write_size___ > 0)\
	{\
		size_t sz___ = size_inside_page(phy_addr___, write_size___);\
\
		/*
		 * On ia64 if a page has been mapped somewhere as uncached, then
		 * it must also be accessed uncached by the kernel or data
		 * corruption may occur.
		 */\
\
		char *ptr___ = xlate_dev_mem_ptr(phy_addr___);\
		if (!ptr___)\
		{\
			printk_debug(KERN_INFO "Error in xlate_dev_mem_ptr:0x%zx\n", phy_addr___);\
			RETURN_VALUE(ret___, realWrite___) \
		}\
\
		if (is_kernel_buf___)\
		{\
			memcpy(ptr___, lpBuf___, sz___); \
		}\
		else\
		{\
			unsigned long copied___ = copy_from_user(ptr___, lpBuf___, sz___); \
		\
			if (copied___)\
			{\
				unxlate_dev_mem_ptr(phy_addr___, ptr___); \
				realWrite___ += sz___ - copied___; \
				printk_debug(KERN_INFO "Error in copy_from_user\n"); \
				RETURN_VALUE(ret___, realWrite___) \
			}\
		}\
		unxlate_dev_mem_ptr(phy_addr___, ptr___);\
		lpBuf___ += sz___;\
		phy_addr___ += sz___;\
		write_size___ -= sz___;\
		realWrite___ += sz___;\
	}\
	RETURN_VALUE(ret___, realWrite___) \
} while (0)



#endif /* PHY_MEM_H_ */