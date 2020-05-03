#include <linux/mm.h>
#include <linux/err.h>
#include <linux/security.h>
#include <linux/slab.h> //kmalloc与kfree
#include "remote_proc_maps.h"
#include "version_control.h"

size_t get_proc_map_count(struct pid* proc_pid_struct)
{
	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return 0;
	}
	struct mm_struct *mm;
	size_t count = 0;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return 0;
	}
	down_read(&mm->mmap_sem);
	count = mm->map_count;
	up_read(&mm->mmap_sem);
	mmput(mm);

	return count;
}

bool check_proc_map_can_read(struct pid* proc_pid_struct, size_t proc_virt_addr, size_t size)
{
	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	bool res = false;

	if (!task)
	{
		return res;
	}
	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return res;
	}
	down_read(&mm->mmap_sem);

	vma = find_vma(mm, proc_virt_addr);
	if (vma)
	{
		if (vma->vm_flags & VM_READ)
		{
			size_t read_end = proc_virt_addr + size;
			if (read_end <= vma->vm_end)
			{
				res = true;
			}
		}
	}
	up_read(&mm->mmap_sem);
	mmput(mm);
	return res;
}
bool check_proc_map_can_write(struct pid* proc_pid_struct, size_t proc_virt_addr, size_t size)
{
	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	bool res = false;
	if (!task)
	{
		return res;
	}
	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return res;
	}
	down_read(&mm->mmap_sem);

	vma = find_vma(mm, proc_virt_addr);
	if (vma)
	{
		if (vma->vm_flags & VM_WRITE)
		{
			size_t read_end = proc_virt_addr + size;
			if (read_end <= vma->vm_end)
			{
				res = true;
			}
		}
	}
	up_read(&mm->mmap_sem);
	mmput(mm);
	return res;
}


//已核对
#ifdef LINUX_VERSION_3_10_0

/* Check if the vma is being used as a stack by this task */
static int vm_is_stack_for_task(struct task_struct *t,
	struct vm_area_struct *vma)
{
	return (vma->vm_start <= KSTK_ESP(t) && vma->vm_end >= KSTK_ESP(t));
}

/*
 * Check if the vma is being used as a stack.
 * If is_group is non-zero, check in the entire thread group or else
 * just check in the current task. Returns the pid of the task that
 * the vma is stack for.
 */
pid_t vm_is_stack(struct task_struct *task,
	struct vm_area_struct *vma, int in_group)
{
	pid_t ret = 0;

	if (vm_is_stack_for_task(task, vma))
		return task->pid;

	if (in_group) {
		struct task_struct *t;
		rcu_read_lock();
		if (!pid_alive(task))
			goto done;

		t = task;
		do {
			if (vm_is_stack_for_task(t, vma)) {
				ret = t->pid;
				goto done;
			}
		} while_each_thread(task, t);
	done:
		rcu_read_unlock();
	}

	return ret;
}

int get_proc_maps_list_to_kernel(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int *have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];

	memset(lpBuf, 0, buf_size);

	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;

		/* We don't show the stack guard page in /proc/maps */
		start = vma->vm_start;
		if (stack_guard_page_start(vma, start))
			start += PAGE_SIZE;
		end = vma->vm_end;
		if (stack_guard_page_end(vma, end))
			end -= PAGE_SIZE;

		memcpy(copy_pos, &start, 8);
		copy_pos += 8;
		memcpy(copy_pos, &end, 8);
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		memcpy(copy_pos, &flags, 4);
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				pid_t tid = vm_is_stack(task, vma, 1);
				if (tid != 0)
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */

					sprintf(new_path, "[stack:%d]", tid);
				}
			}

		}
		memcpy(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path));
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);

	return success;
}


int get_proc_maps_list_to_user(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int * have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];


	//清空用户的缓冲区
	if (clear_user(lpBuf, buf_size))
	{
		return -4;
	}
	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;

		/* We don't show the stack guard page in /proc/maps */
		start = vma->vm_start;
		if (stack_guard_page_start(vma, start))
			start += PAGE_SIZE;
		end = vma->vm_end;
		if (stack_guard_page_end(vma, end))
			end -= PAGE_SIZE;


		//内核空间->用户空间交换数据
		if (copy_to_user(copy_pos, &start, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		if (copy_to_user(copy_pos, &end, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		if (copy_to_user(copy_pos, &flags, 4))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				pid_t tid = vm_is_stack(task, vma, 1);
				if (tid != 0)
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */

					sprintf(new_path, "[stack:%d]", tid);
				}
			}

		}
		if (copy_to_user(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path)))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);
	return success;
}

#endif

//已核对
#ifdef LINUX_VERSION_3_10_84
/* Check if the vma is being used as a stack by this task */
static int vm_is_stack_for_task(struct task_struct *t,
	struct vm_area_struct *vma)
{
	return (vma->vm_start <= KSTK_ESP(t) && vma->vm_end >= KSTK_ESP(t));
}

/*
 * Check if the vma is being used as a stack.
 * If is_group is non-zero, check in the entire thread group or else
 * just check in the current task. Returns the pid of the task that
 * the vma is stack for.
 */
pid_t vm_is_stack(struct task_struct *task,
	struct vm_area_struct *vma, int in_group)
{
	pid_t ret = 0;

	if (vm_is_stack_for_task(task, vma))
		return task->pid;

	if (in_group) {
		struct task_struct *t;
		rcu_read_lock();
		if (!pid_alive(task))
			goto done;

		t = task;
		do {
			if (vm_is_stack_for_task(t, vma)) {
				ret = t->pid;
				goto done;
			}
		} while_each_thread(task, t);
	done:
		rcu_read_unlock();
	}

	return ret;
}

int get_proc_maps_list_to_kernel(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int *have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];
	char path_buf[PATH_MAX];

	memset(lpBuf, 0, buf_size);

	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;

		/* We don't show the stack guard page in /proc/maps */
		start = vma->vm_start;
		if (stack_guard_page_start(vma, start))
			start += PAGE_SIZE;
		end = vma->vm_end;
		if (stack_guard_page_end(vma, end))
			end -= PAGE_SIZE;

		memcpy(copy_pos, &start, 8);
		copy_pos += 8;
		memcpy(copy_pos, &end, 8);
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		memcpy(copy_pos, &flags, 4);
		copy_pos += 4;

		memset(path_buf, 0, sizeof(path_buf));
		if (vma->vm_file)
		{
			memset(new_path, 0, sizeof(new_path));
			char *path = d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
			strcat(path_buf, path);
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(path_buf, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(path_buf, "[heap]");
			}
			else
			{
				pid_t tid = vm_is_stack(task, vma, 1);
				if (tid != 0)
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */

					sprintf(path_buf, "[stack:%d]", tid);
				}
			}

		}
		memcpy(copy_pos, &path_buf, max_path_length < sizeof(path_buf) ? max_path_length - 1 : sizeof(path_buf));
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);

	return success;
}


int get_proc_maps_list_to_user(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int * have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];
	char path_buf[PATH_MAX];

	//清空用户的缓冲区
	if (clear_user(lpBuf, buf_size))
	{
		return -4;
	}
	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;

		/* We don't show the stack guard page in /proc/maps */
		start = vma->vm_start;
		if (stack_guard_page_start(vma, start))
			start += PAGE_SIZE;
		end = vma->vm_end;
		if (stack_guard_page_end(vma, end))
			end -= PAGE_SIZE;


		//内核空间->用户空间交换数据
		if (copy_to_user(copy_pos, &start, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		if (copy_to_user(copy_pos, &end, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		if (copy_to_user(copy_pos, &flags, 4))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 4;

		memset(path_buf, 0, sizeof(path_buf));
		if (vma->vm_file)
		{
			memset(new_path, 0, sizeof(new_path));
			char *path = d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
			strcat(path_buf, path);
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(path_buf, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(path_buf, "[heap]");
			}
			else
			{
				pid_t tid = vm_is_stack(task, vma, 1);
				if (tid != 0)
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */

					sprintf(path_buf, "[stack:%d]", tid);
				}
			}

		}
		if (copy_to_user(copy_pos, &path_buf, max_path_length < sizeof(path_buf) ? max_path_length - 1 : sizeof(path_buf)))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);
	return success;
}

#endif

//已核对
#ifdef LINUX_VERSION_3_18_71
/* Check if the vma is being used as a stack by this task */
static int vm_is_stack_for_task(struct task_struct *t,
	struct vm_area_struct *vma)
{
	return (vma->vm_start <= KSTK_ESP(t) && vma->vm_end >= KSTK_ESP(t));
}


/*
 * Check if the vma is being used as a stack.
 * If is_group is non-zero, check in the entire thread group or else
 * just check in the current task. Returns the task_struct of the task
 * that the vma is stack for. Must be called under rcu_read_lock().
 */
struct task_struct *task_of_stack(struct task_struct *task,
	struct vm_area_struct *vma, bool in_group)
{
	if (vm_is_stack_for_task(task, vma))
		return task;

	if (in_group) {
		struct task_struct *t;

		for_each_thread(task, t) {
			if (vm_is_stack_for_task(t, vma))
				return t;
		}
	}

	return NULL;
}


static pid_t pid_of_stack(struct task_struct *task,
	struct vm_area_struct *vma, bool is_pid)
{
	pid_t ret = 0;

	rcu_read_lock();
	task = task_of_stack(task, vma, is_pid);
	if (task)
	{
		ret = task->pid;
	}
	rcu_read_unlock();

	return ret;
}


int get_proc_maps_list_to_kernel(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int *have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];

	memset(lpBuf, 0, buf_size);

	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;

		memcpy(copy_pos, &start, 8);
		copy_pos += 8;
		memcpy(copy_pos, &end, 8);
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		memcpy(copy_pos, &flags, 4);
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				pid_t tid = pid_of_stack(task, vma, 1);
				if (tid != 0)
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					if (vma->vm_start <= mm->start_stack &&
						vma->vm_end >= mm->start_stack) {
						strcat(new_path, "[stack]");
					}
					else {
						sprintf(new_path, "[stack:%d]", tid);
					}
				}

			}

		}
		memcpy(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path));
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);

	return success;
}


int get_proc_maps_list_to_user(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int * have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];


	//清空用户的缓冲区
	if (clear_user(lpBuf, buf_size))
	{
		return -3;
	}
	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;

		//内核空间->用户空间交换数据
		if (copy_to_user(copy_pos, &start, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		if (copy_to_user(copy_pos, &end, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		if (copy_to_user(copy_pos, &flags, 4))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				pid_t tid = pid_of_stack(task, vma, 1);
				if (tid != 0)
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					if (vma->vm_start <= mm->start_stack &&
						vma->vm_end >= mm->start_stack) {
						strcat(new_path, "[stack]");
					}
					else {
						sprintf(new_path, "[stack:%d]", tid);
					}
				}
			}

		}
		if (copy_to_user(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path)))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);
	return success;
}



#endif

//已核对
#ifdef LINUX_VERSION_4_4_78
/* Check if the vma is being used as a stack by this task */
int vma_is_stack_for_task(struct vm_area_struct *vma, struct task_struct *t)
{
	return (vma->vm_start <= KSTK_ESP(t) && vma->vm_end >= KSTK_ESP(t));
}

/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct task_struct *task,
	struct vm_area_struct *vma, int is_pid)
{
	int stack = 0;

	if (is_pid) {
		stack = vma->vm_start <= vma->vm_mm->start_stack &&
			vma->vm_end >= vma->vm_mm->start_stack;
	}
	else {
		rcu_read_lock();
		stack = vma_is_stack_for_task(vma, task);
		rcu_read_unlock();
	}
	return stack;
}


int get_proc_maps_list_to_kernel(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int *have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];

	memset(lpBuf, 0, buf_size);

	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;


		memcpy(copy_pos, &start, 8);
		copy_pos += 8;
		memcpy(copy_pos, &end, 8);
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		memcpy(copy_pos, &flags, 4);
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				pid_t tid = is_stack(task, vma, 1);
				if (tid != 0)
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					sprintf(new_path, "[stack:%d]", tid);
				}

			}

		}
		memcpy(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path));
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);

	return success;
}


int get_proc_maps_list_to_user(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int * have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];


	//清空用户的缓冲区
	if (clear_user(lpBuf, buf_size))
	{
		return -3;
	}
	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;

		//内核空间->用户空间交换数据
		if (copy_to_user(copy_pos, &start, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		if (copy_to_user(copy_pos, &end, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		if (copy_to_user(copy_pos, &flags, 4))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				pid_t tid = is_stack(task, vma, 1);
				if (tid != 0)
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					sprintf(new_path, "[stack:%d]", tid);
				}
			}

		}
		if (copy_to_user(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path)))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);
	return success;
}



#endif



//已核对
#ifdef LINUX_VERSION_4_4_153
/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct task_struct *task,
	struct vm_area_struct *vma)
{
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}


int get_proc_maps_list_to_kernel(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int *have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];

	memset(lpBuf, 0, buf_size);

	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;

		memcpy(copy_pos, &start, 8);
		copy_pos += 8;
		memcpy(copy_pos, &end, 8);
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		memcpy(copy_pos, &flags, 4);
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				pid_t tid = is_stack(task, vma);
				if (tid != 0)
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					strcat(new_path, "[stack]");
				}

			}

		}
		memcpy(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path));
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);

	return success;
}


int get_proc_maps_list_to_user(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int * have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];


	//清空用户的缓冲区
	if (clear_user(lpBuf, buf_size))
	{
		return -4;
	}
	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;

		//内核空间->用户空间交换数据
		if (copy_to_user(copy_pos, &start, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		if (copy_to_user(copy_pos, &end, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		if (copy_to_user(copy_pos, &flags, 4))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				pid_t tid = is_stack(task, vma);
				if (tid != 0)
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					strcat(new_path, "[stack]");
				}
			}

		}
		if (copy_to_user(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path)))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);
	return success;
}
#endif


//已核对
#ifdef LINUX_VERSION_4_4_192
/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct task_struct *task,
	struct vm_area_struct *vma)
{
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}


int get_proc_maps_list_to_kernel(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int *have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];

	memset(lpBuf, 0, buf_size);

	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;

		memcpy(copy_pos, &start, 8);
		copy_pos += 8;
		memcpy(copy_pos, &end, 8);
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		memcpy(copy_pos, &flags, 4);
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				pid_t tid = is_stack(task, vma);
				if (tid != 0)
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					strcat(new_path, "[stack]");
				}

			}

		}
		memcpy(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path));
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);

	return success;
}


int get_proc_maps_list_to_user(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int * have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];


	//清空用户的缓冲区
	if (clear_user(lpBuf, buf_size))
	{
		return -4;
	}
	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;

		//内核空间->用户空间交换数据
		if (copy_to_user(copy_pos, &start, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		if (copy_to_user(copy_pos, &end, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		if (copy_to_user(copy_pos, &flags, 4))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				pid_t tid = is_stack(task, vma);
				if (tid != 0)
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					strcat(new_path, "[stack]");
				}
			}

		}
		if (copy_to_user(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path)))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);
	return success;
}
#endif


//已核对
#ifdef LINUX_VERSION_4_9_112
/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct proc_maps_private *priv,
	struct vm_area_struct *vma)
{
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}



int get_proc_maps_list_to_kernel(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int *have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];

	memset(lpBuf, 0, buf_size);

	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;


		memcpy(copy_pos, &start, 8);
		copy_pos += 8;
		memcpy(copy_pos, &end, 8);
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		memcpy(copy_pos, &flags, 4);
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				if (is_stack(task, vma))
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					strcat(new_path, "[stack]");
				}

			}

		}
		memcpy(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path));
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);

	return success;
}


int get_proc_maps_list_to_user(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int * have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];


	//清空用户的缓冲区
	if (clear_user(lpBuf, buf_size))
	{
		return -4;
	}
	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;


		//内核空间->用户空间交换数据
		if (copy_to_user(copy_pos, &start, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		if (copy_to_user(copy_pos, &end, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		if (copy_to_user(copy_pos, &flags, 4))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				if (is_stack(task, vma))
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					strcat(new_path, "[stack]");
				}
			}

		}
		if (copy_to_user(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path)))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);
	return success;
}

#endif

//已核对
#ifdef LINUX_VERSION_4_14_83
/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct vm_area_struct *vma)
{
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

int get_proc_maps_list_to_kernel(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int *have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];

	memset(lpBuf, 0, buf_size);

	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;


		memcpy(copy_pos, &start, 8);
		copy_pos += 8;
		memcpy(copy_pos, &end, 8);
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		memcpy(copy_pos, &flags, 4);
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				if (is_stack(vma))
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					strcat(new_path, "[stack]");
				}

			}

		}
		memcpy(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path));
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);

	return success;
}


int get_proc_maps_list_to_user(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int * have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];


	//清空用户的缓冲区
	if (clear_user(lpBuf, buf_size))
	{
		return -4;
	}
	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;


		//内核空间->用户空间交换数据
		if (copy_to_user(copy_pos, &start, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		if (copy_to_user(copy_pos, &end, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		if (copy_to_user(copy_pos, &flags, 4))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				if (is_stack(vma))
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					strcat(new_path, "[stack]");
				}
			}

		}
		if (copy_to_user(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path)))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);
	return success;


}
#endif


//已核对
#ifdef LINUX_VERSION_4_14_117


/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct vm_area_struct *vma)
{
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

int get_proc_maps_list_to_kernel(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int *have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];

	memset(lpBuf, 0, buf_size);

	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;


		memcpy(copy_pos, &start, 8);
		copy_pos += 8;
		memcpy(copy_pos, &end, 8);
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		memcpy(copy_pos, &flags, 4);
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				if (is_stack(vma))
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					strcat(new_path, "[stack]");
				}

			}

		}
		memcpy(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path));
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);

	return success;
}


int get_proc_maps_list_to_user(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int * have_pass)
{
	if (max_path_length > PATH_MAX || max_path_length <= 0)
	{
		return -1;
	}

	struct task_struct *task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task)
	{
		return -2;
	}
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
	{
		return -3;
	}

	char new_path[PATH_MAX];


	//清空用户的缓冲区
	if (clear_user(lpBuf, buf_size))
	{
		return -4;
	}
	int success = 0;

	size_t copy_pos = (size_t)lpBuf;
	size_t end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next)
	{
		if (copy_pos >= end_pos)
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		unsigned long start, end;
		start = vma->vm_start;
		end = vma->vm_end;


		//内核空间->用户空间交换数据
		if (copy_to_user(copy_pos, &start, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		if (copy_to_user(copy_pos, &end, 8))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 8;

		unsigned char flags[4];
		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';
		if (copy_to_user(copy_pos, &flags, 4))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += 4;

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file)
		{
			d_path(&vma->vm_file->f_path, new_path, sizeof(new_path));
		}
		else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		{
			strcat(new_path, "[vdso]");
		}
		else
		{
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk)
			{
				strcat(new_path, "[heap]");
			}
			else
			{
				if (is_stack(vma))
				{
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					strcat(new_path, "[stack]");
				}
			}

		}
		if (copy_to_user(copy_pos, &new_path, max_path_length < sizeof(new_path) ? max_path_length - 1 : sizeof(new_path)))
		{
			if (have_pass)
			{
				*have_pass = 1;
			}
			break;
		}
		copy_pos += max_path_length;
		success++;
	}
	up_read(&mm->mmap_sem);
	mmput(mm);
	return success;


}
#endif

