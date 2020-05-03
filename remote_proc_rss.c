#include <linux/ksm.h>
#include <linux/pid.h>
#include "version_control.h"
size_t read_proc_rss_size(struct pid* proc_pid_struct, ssize_t relative_offset)
{
	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return 0;
	}
	struct mm_struct *mm = get_task_mm(task);
	put_task_struct(task);
	if (mm)
	{
		//精确偏移
		ssize_t accurate_offset = (ssize_t)((size_t)&mm->rss_stat - (size_t)mm + relative_offset);
		if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t))
		{
			mmput(mm);
			return 0;
		}


		size_t total_rss = 0;
		struct mm_rss_stat *rss_stat = (struct mm_rss_stat*)((size_t)mm + (size_t)accurate_offset);

		ssize_t val1 = atomic_long_read(&rss_stat->count[MM_FILEPAGES]);
		if (val1 > 0)
		{
			ssize_t val2 = atomic_long_read(&rss_stat->count[MM_ANONPAGES]);
			if (val2 > 0)
			{
				total_rss = val1 + val2;
			}
		}
		mmput(mm);
		return total_rss;
	}
	return 0;

}