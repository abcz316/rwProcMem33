#ifndef PROC_CMDLINE_AUTO_OFFSET_H_
#define PROC_CMDLINE_AUTO_OFFSET_H_
//声明
//////////////////////////////////////////////////////////////////////////
#include <linux/pid.h>
#include "ver_control.h"
//实现
//////////////////////////////////////////////////////////////////////////
#include "api_proxy.h"
static ssize_t g_arg_start_offset = 0;
static bool g_init_arg_start_offset_success = false;

typedef int(*t_get_task_proc_cmdline_addr)(struct task_struct *task, size_t * arg_start, size_t * arg_end);

static inline int init_proc_cmdline_offset(const char* my_cmdline,
	t_get_task_proc_cmdline_addr o_get_task_proc_cmdline_addr) {

	int is_found_cmdline_offset = 0;
	size_t size = 4096;
	char *new_cmd_line_buf = NULL;
	struct task_struct * mytask = x_get_current();

	if(g_init_arg_start_offset_success) {
		return 0;
	}

	new_cmd_line_buf = (char*)kmalloc(size, GFP_KERNEL);
	g_init_arg_start_offset_success = true;
	for (g_arg_start_offset = -64; g_arg_start_offset <= 64; g_arg_start_offset += 1) {
		size_t arg_start = 0, arg_end = 0;
		printk_debug(KERN_INFO "get_task_proc_cmdline_addr g_arg_start_offset %zd\n", g_arg_start_offset);
		if (o_get_task_proc_cmdline_addr(mytask, &arg_start, &arg_end) == 0) {
			printk_debug(KERN_INFO "get_task_proc_cmdline_addr arg_start %p\n", (void*)arg_start);

			if (arg_start > 0) {

				size_t read_size = 0;

				memset(new_cmd_line_buf, 0, size);

				while (read_size < size) {
					size_t phy_addr;
					size_t pfn_sz;
					char *lpOutBuf;

					pte_t *pte;
					phy_addr = get_task_proc_phy_addr(mytask, arg_start + read_size, (pte_t*)&pte);
					printk_debug(KERN_INFO "phy_addr:0x%zx\n", phy_addr);
					if (phy_addr == 0) {
						break;
					}

					pfn_sz = size_inside_page(phy_addr, ((size - read_size) > PAGE_SIZE) ? PAGE_SIZE : (size - read_size));
					printk_debug(KERN_INFO "pfn_sz:%zu\n", pfn_sz);

					lpOutBuf = (char*)(new_cmd_line_buf + read_size);
					read_ram_physical_addr(true, phy_addr, lpOutBuf, pfn_sz);
					read_size += pfn_sz;
				}

				printk_debug(KERN_INFO "new_cmd_line_buf:%s, len:%ld\n", new_cmd_line_buf, strlen(new_cmd_line_buf));

				if (strcmp(new_cmd_line_buf, my_cmdline) == 0) {
					is_found_cmdline_offset = 1;
					break;
				}


			}
		}
	}

	kfree(new_cmd_line_buf);

	if (!is_found_cmdline_offset) {
		g_init_arg_start_offset_success = false;
		printk_debug(KERN_INFO "find cmdline offset failed\n");
		return -ESPIPE;
	}
	printk_debug(KERN_INFO "g_arg_start_offset:%zu\n", g_arg_start_offset);
	return 0;
}
#endif /* PROC_CMDLINE_AUTO_OFFSET_H_ */