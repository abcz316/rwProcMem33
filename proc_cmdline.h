#ifndef PROC_CMDLINE_H_
#define PROC_CMDLINE_H_
//声明
//////////////////////////////////////////////////////////////////////////
#include <linux/pid.h>
#include "ver_control.h"

MY_STATIC inline struct pid * get_proc_pid_struct(int pid);
MY_STATIC inline int get_proc_pid(struct pid* proc_pid_struct);
MY_STATIC inline void release_proc_pid_struct(struct pid* proc_pid_struct);
MY_STATIC inline int get_proc_cmdline_addr(struct pid* proc_pid_struct, size_t * arg_start, size_t * arg_end);
MY_STATIC inline int get_task_proc_cmdline_addr(struct task_struct *task, size_t * arg_start, size_t * arg_end);


//实现
//////////////////////////////////////////////////////////////////////////
#include "phy_mem.h"
#include "proc_cmdline_auto_offset.h"
#include "api_proxy.h"

MY_STATIC inline struct pid * get_proc_pid_struct(int pid) {
	return find_get_pid(pid);
}

MY_STATIC inline int get_proc_pid(struct pid* proc_pid_struct) {
	return proc_pid_struct->numbers[0].nr;
}
MY_STATIC inline void release_proc_pid_struct(struct pid* proc_pid_struct) {
	put_pid(proc_pid_struct);
}

MY_STATIC inline int get_proc_cmdline_addr(struct pid* proc_pid_struct, size_t * arg_start, size_t * arg_end) {
	int ret = 0;
	struct task_struct *task = NULL;


	if (g_init_arg_start_offset_success == false) {
		return -ENOENT;
	}


	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) { return -EFAULT; }
	ret = get_task_proc_cmdline_addr(task, arg_start, arg_end);
	return ret;
}
MY_STATIC inline int get_task_proc_cmdline_addr(struct task_struct *task, size_t * arg_start, size_t * arg_end) {
	if (g_init_arg_start_offset_success) {
		struct mm_struct *mm;
		ssize_t accurate_offset;
		mm = get_task_mm(task);

		if (!mm) { return -EFAULT; }

		//精确偏移
		accurate_offset = (ssize_t)((size_t)&mm->arg_start - (size_t)mm + g_arg_start_offset_proc_cmdline);
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
	return -ESPIPE;
}

#endif /* PROC_CMDLINE_H_ */