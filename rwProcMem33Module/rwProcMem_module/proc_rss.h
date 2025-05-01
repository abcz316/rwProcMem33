#ifndef PROC_RSS_H_
#define PROC_RSS_H_
//声明
//////////////////////////////////////////////////////////////////////////
#include <linux/pid.h>
#include "api_proxy.h"
#include "ver_control.h"
MY_STATIC size_t read_proc_rss_size(struct pid* proc_pid_struct);

//实现
//////////////////////////////////////////////////////////////////////////
#include "proc_cmdline_auto_offset.h"
MY_STATIC size_t read_proc_rss_size(struct pid* proc_pid_struct) {
	struct task_struct *task;
	struct mm_struct *mm;
	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return 0;
	}
	mm = get_task_mm(task);
	if (mm) {
		//精确偏移
		size_t total_rss;
		ssize_t offset = g_init_arg_start_offset_success ? g_arg_start_offset_proc_cmdline : 0;
		//struct mm_rss_stat *rss_stat = (struct mm_rss_stat *)((size_t)&mm->rss_stat + offset);
		//printk_debug(KERN_INFO "&mm->rss_stat:%p,rss_stat:%p\n", (void*)&mm->rss_stat, (void*)rss_stat);
		total_rss = x_read_mm_struct_rss(mm, offset);
		mmput(mm);
		return total_rss;
	}
	return 0;

}
#endif /* PROC_RSS_H_ */