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

struct mm_struct_arg_offset_helper {
	unsigned long arg_start, arg_end, env_start, env_end;
};

static inline int init_proc_cmdline_offset(const char* my_auxv, int my_auxv_size) {
    struct task_struct *mytask = x_get_current();
    struct mm_struct  *mm = NULL;
	const unsigned char *base = NULL;
	size_t limit = 0;
	size_t off = 0;
    if (g_init_arg_start_offset_success) {
        return 0;
    }

    mm = get_task_mm(mytask);
    if (!mm) {
        return -EFAULT;
    }

    base  = (const unsigned char *)mm;
    limit = sizeof(struct mm_struct);

    for (off = 0; off + (size_t)my_auxv_size <= limit; ++off) {
        size_t i = 0;
        for (; i < (size_t)my_auxv_size; ++i) {
            if (base[off + i] != (unsigned char)my_auxv[i]) {
                break;
            }
        }
        if (i == (size_t)my_auxv_size) {
            g_arg_start_offset = (ssize_t)off - sizeof(struct mm_struct_arg_offset_helper) + (ssize_t)offsetof(struct mm_struct_arg_offset_helper, arg_start) - (ssize_t)offsetof(struct mm_struct, arg_start);
            g_init_arg_start_offset_success = true;
            break;
        }
    }
    mmput(mm);
	if (!g_init_arg_start_offset_success) {
		printk_debug(KERN_INFO "find auxv offset failed\n");
		return -ESPIPE;
	}
	printk_debug(KERN_INFO "g_arg_start_offset:%zd\n", g_arg_start_offset);
    return 0;
}
#endif /* PROC_CMDLINE_AUTO_OFFSET_H_ */