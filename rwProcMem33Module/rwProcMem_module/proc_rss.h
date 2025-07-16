#ifndef PROC_RSS_H_
#define PROC_RSS_H_
//声明
//////////////////////////////////////////////////////////////////////////
#include <linux/pid.h>
#include "api_proxy.h"
#include "ver_control.h"
static size_t read_proc_rss_size(struct pid* proc_pid_struct);

//实现
//////////////////////////////////////////////////////////////////////////
#include "proc_cmdline_auto_offset.h"
static size_t read_proc_rss_size(struct pid* proc_pid_struct) {
	struct task_struct *task;
	struct mm_struct *mm;
	size_t total_rss;
	ssize_t offset;
	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return 0;
	}
	mm = get_task_mm(task);
	if(!mm) {
		return 0;
	}
	offset = g_init_arg_start_offset_success ? g_arg_start_offset : 0;
	total_rss = x_read_mm_struct_rss(mm, offset);
	mmput(mm);
	return total_rss;

}
#endif /* PROC_RSS_H_ */
