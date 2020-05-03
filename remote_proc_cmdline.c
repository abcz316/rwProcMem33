#include <linux/ksm.h>
#include <linux/pid.h>
#include "version_control.h"


struct pid * get_proc_pid_struct(int nPid)
{
	return find_get_pid(nPid);
}
int get_proc_pid(struct pid* proc_pid_struct)
{
	return proc_pid_struct->numbers[0].nr;
}
void release_proc_pid_struct(struct pid* proc_pid_struct)
{
	put_pid(proc_pid_struct);
}

int get_proc_cmdline_addr(struct pid* proc_pid_struct, ssize_t relative_offset, size_t * arg_start, size_t * arg_end)
{
	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -1;
	}
	struct mm_struct *mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -1;
	}

	//精确偏移
	ssize_t accurate_offset = (ssize_t)((size_t)&mm->arg_start - (size_t)mm + relative_offset);
	if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t))
	{
		mmput(mm);
		return 0;
	}


	down_read(&mm->mmap_sem);
	printk_debug(KERN_INFO "accurate_offset:%ld\n", accurate_offset);
	*arg_start = *(size_t*)((size_t)mm + (size_t)accurate_offset);
	printk_debug(KERN_INFO "arg_start addr:0x%llx\n", *arg_start);

	*arg_end = *(size_t*)((size_t)mm + (size_t)accurate_offset + sizeof(unsigned long));
	up_read(&mm->mmap_sem);
	mmput(mm);
	return 0;
}
