#ifndef PROC_CMDLINE_H_
#define PROC_CMDLINE_H_
//声明
//////////////////////////////////////////////////////////////////////////
#include <linux/pid.h>
#include "ver_control.h"

static inline struct pid * get_proc_pid_struct(int pid);
static inline int get_proc_pid(struct pid* proc_pid_struct);
static inline void release_proc_pid_struct(struct pid* proc_pid_struct);
static inline int get_proc_cmdline_addr(struct pid* proc_pid_struct, size_t * arg_start, size_t * arg_end);

//实现
//////////////////////////////////////////////////////////////////////////
#include "phy_mem.h"
#include "proc_cmdline_auto_offset.h"
#include "api_proxy.h"

static inline struct pid * get_proc_pid_struct(int pid) {
	return find_get_pid(pid);
}
static inline int get_proc_pid(struct pid* proc_pid_struct) {
	return proc_pid_struct->numbers[0].nr;
}
static inline void release_proc_pid_struct(struct pid* proc_pid_struct) {
	put_pid(proc_pid_struct);
}

static inline int get_proc_cmdline_addr(struct pid* proc_pid_struct, size_t * arg_start, size_t * arg_end) {
	struct task_struct *task = NULL;
	struct mm_struct *mm = NULL;
	ssize_t accurate_offset = 0;

	if (g_init_arg_start_offset_success == false) {
		return -ENOENT;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) { return -EFAULT; }
	
	mm = get_task_mm(task);

	if (!mm) { return -EFAULT; }

	//精确偏移
	accurate_offset = (ssize_t)((size_t)&mm->arg_start - (size_t)mm + g_arg_start_offset);
	if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t)) {
		mmput(mm);
		return -EFAULT;
	}

	if (down_read_mmap_lock(mm) != 0) {
		mmput(mm);
		return -EFAULT;
	}
	printk_debug(KERN_INFO "accurate_offset:%zd\n", accurate_offset);

	*arg_start = *(size_t*)((size_t)mm + (size_t)accurate_offset);
	*arg_end = *(size_t*)((size_t)mm + (size_t)accurate_offset + sizeof(unsigned long));

	printk_debug(KERN_INFO "arg_start addr:0x%p\n", (void*)*arg_start);

	up_read_mmap_lock(mm);
	mmput(mm);
	return 0;
}
#endif /* PROC_CMDLINE_H_ */