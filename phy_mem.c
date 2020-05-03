#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/highmem.h>
#include <linux/slab.h> 
#include "phy_mem.h"
#include "version_control.h"
#include "remote_proc_cmdline.h"

#define PAGEMAP_ENTRY sizeof(uint64_t)
#define GET_BIT(X, Y) (X & ((uint64_t)1 << Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF
const int __endian_bit = 1;
#define is_bigendian() ( (*(char*)&__endian_bit) == 0 )

struct file * open_pagemap(int pid)
{
	struct file *filp = NULL;

	char szFilePath[256];
	memset(szFilePath, 0, sizeof(szFilePath));
	snprintf(szFilePath, sizeof(szFilePath), "/proc/%d/pagemap", pid);
	filp = filp_open(szFilePath, O_RDONLY, 0);
	if (IS_ERR(filp))
	{
		return NULL;
	}
	return filp;
}
size_t get_pagemap_phy_addr(struct file * lpPagemap, size_t virt_addr)
{

	uint64_t page_size = PAGE_SIZE;
	uint64_t file_offset;
	mm_segment_t pold_fs;
	uint64_t read_val;
	unsigned char c_buf[PAGEMAP_ENTRY];
	int i;
	unsigned char c;
	printk_debug(KERN_INFO "page_size %d\n", page_size);
	printk_debug(KERN_INFO "Big endian? %d\n", is_bigendian());

	
	
	file_offset = virt_addr / page_size * PAGEMAP_ENTRY;

	printk_debug(KERN_INFO "Vaddr: 0x%lx, Page_size: %lld, Entry_size: %d\n", virt_addr, page_size, PAGEMAP_ENTRY);

	printk_debug(KERN_INFO "Reading at 0x%llx\n", (unsigned long long) file_offset);

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
	printk_debug(KERN_INFO "Result: 0x%llx\n", (unsigned long long) read_val);

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
void close_pagemap(struct file* lpPagemap)
{
	filp_close(lpPagemap, NULL);
}

size_t get_proc_phy_addr(struct pid* proc_pid_struct, size_t virt_addr)
{
	struct file * pFile = open_pagemap(get_proc_pid(proc_pid_struct));
	size_t phy_addr = 0;
	printk_debug(KERN_INFO "open_pagemap %d\n", pFile);
	if (pFile)
	{
		phy_addr = get_pagemap_phy_addr(pFile, virt_addr);
		close_pagemap(pFile);
	}
	return phy_addr;
}

static inline int __valid_phys_addr_range(size_t addr, size_t count)
{
	return addr + count <= __pa(high_memory);
}


#ifndef xlate_dev_mem_ptr
#define xlate_dev_mem_ptr(p) __va(p)
#endif

#ifndef unxlate_dev_mem_ptr
#define unxlate_dev_mem_ptr unxlate_dev_mem_ptr
void __weak unxlate_dev_mem_ptr(unsigned long phys, void *addr)
{
}
#endif


size_t read_ram_physical_addr_to_kernel(size_t phy_addr, char* lpBuf, size_t read_size)
{
	if (!__valid_phys_addr_range(phy_addr, read_size))
	{
		printk_debug(KERN_INFO "Error in valid_phys_addr_range:0x%llx,size:%ld\n", phy_addr, read_size);
		return 0;
	}
	char *bounce = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!bounce)
	{
		return 0;
	}
	size_t realRead = 0;
	while (read_size > 0)
	{
		size_t sz = size_inside_page(phy_addr, read_size);

		/*
		 * On ia64 if a page has been mapped somewhere as uncached, then
		 * it must also be accessed uncached by the kernel or data
		 * corruption may occur.
		 */

		char *ptr = xlate_dev_mem_ptr(phy_addr);
		if (!ptr)
		{
			printk_debug(KERN_INFO "Error in xlate_dev_mem_ptr:0x%llx\n", phy_addr);
			kfree(bounce);
			return realRead;
		}
		int probe = probe_kernel_read(bounce, ptr, sz);
		unxlate_dev_mem_ptr(phy_addr, ptr);
		if (probe)
		{
			kfree(bounce);
			return realRead;
		}
		memcpy(lpBuf, bounce, sz);


		lpBuf += sz;
		phy_addr += sz;
		read_size -= sz;
		realRead += sz;
	}
	kfree(bounce);
	return realRead;
}
size_t read_ram_physical_addr_to_user(size_t phy_addr, char* lpBuf, size_t read_size)
{
	if (!__valid_phys_addr_range(phy_addr, read_size))
	{
		printk_debug(KERN_INFO "Error in valid_phys_addr_range:0x%llx,size:%ld\n", phy_addr, read_size);
		return 0;
	}
	char *bounce = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!bounce)
	{
		return 0;
	}
	size_t realRead = 0;
	while (read_size > 0)
	{
		size_t sz = size_inside_page(phy_addr, read_size);

		/*
		 * On ia64 if a page has been mapped somewhere as uncached, then
		 * it must also be accessed uncached by the kernel or data
		 * corruption may occur.
		 */

		char *ptr = xlate_dev_mem_ptr(phy_addr);
		if (!ptr)
		{
			printk_debug(KERN_INFO "Error in xlate_dev_mem_ptr:0x%llx\n", phy_addr);
			kfree(bounce);
			return realRead;
		}
		int probe = probe_kernel_read(bounce, ptr, sz);
		unxlate_dev_mem_ptr(phy_addr, ptr);
		if (probe)
		{
			kfree(bounce);
			return realRead;
		}
		unsigned long remaining = copy_to_user(lpBuf, bounce, sz);
		if (remaining)
		{
			printk_debug(KERN_INFO "Error in copy_to_user\n");
			kfree(bounce);
			return realRead;
		}


		lpBuf += sz;
		phy_addr += sz;
		read_size -= sz;
		realRead += sz;
	}
	kfree(bounce);
	return realRead;
}
size_t write_ram_physical_addr_from_kernel(size_t phy_addr, char* lpBuf, size_t write_size)
{
	
	if (!__valid_phys_addr_range(phy_addr, write_size))
	{
		printk_debug(KERN_INFO "Error in valid_phys_addr_range:0x%llx,size:%ld\n", phy_addr, write_size);
		return 0;
	}

	size_t realWrite = 0;
	while (write_size > 0)
	{
		size_t sz = size_inside_page(phy_addr, write_size);

		/*
		 * On ia64 if a page has been mapped somewhere as uncached, then
		 * it must also be accessed uncached by the kernel or data
		 * corruption may occur.
		 */

		char *ptr = xlate_dev_mem_ptr(phy_addr);
		if (!ptr)
		{
			printk_debug(KERN_INFO "Error in xlate_dev_mem_ptr:0x%llx\n", phy_addr);
			return realWrite;
		}

		memcpy(ptr, lpBuf, sz);
		unxlate_dev_mem_ptr(phy_addr, ptr);

		lpBuf += sz;
		phy_addr += sz;
		write_size -= sz;
		realWrite += sz;
	}
	return realWrite;
}
size_t write_ram_physical_addr_from_user(size_t phy_addr, char* lpBuf, size_t write_size)
{
	
	if (!__valid_phys_addr_range(phy_addr, write_size))
	{
		printk_debug(KERN_INFO "Error in valid_phys_addr_range:0x%llx,size:%ld\n", phy_addr, write_size);
		return 0;
	}

	size_t realWrite = 0;
	while (write_size > 0)
	{
		size_t sz = size_inside_page(phy_addr, write_size);

		/*
		 * On ia64 if a page has been mapped somewhere as uncached, then
		 * it must also be accessed uncached by the kernel or data
		 * corruption may occur.
		 */

		char *ptr = xlate_dev_mem_ptr(phy_addr);
		if (!ptr)
		{
			printk_debug(KERN_INFO "Error in xlate_dev_mem_ptr:0x%llx\n", phy_addr);
			return realWrite;
		}

		unsigned long copied = copy_from_user(ptr, lpBuf, sz);
		unxlate_dev_mem_ptr(phy_addr, ptr);
		if (copied)
		{
			realWrite += sz - copied;
			printk_debug(KERN_INFO "Error in copy_from_user\n");
			return realWrite;
		}
		lpBuf += sz;
		phy_addr += sz;
		write_size -= sz;
		realWrite += sz;
	}
	return realWrite;
}


