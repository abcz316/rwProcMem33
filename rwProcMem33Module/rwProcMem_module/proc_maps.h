#ifndef PROC_MAPS_H_
#define PROC_MAPS_H_

//声明
//////////////////////////////////////////////////////////////////////////
#include <linux/pid.h>
#include <linux/types.h>
#include <linux/mm_types.h>
#if MY_LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,83)
#include <linux/sched/task.h>
#include <linux/sched/mm.h>
#endif

static inline int down_read_mmap_lock(struct mm_struct *mm);
static inline int up_read_mmap_lock(struct mm_struct *mm);
static inline size_t get_proc_map_count(struct pid* proc_pid_struct);
static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size);

//实现
//////////////////////////////////////////////////////////////////////////
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/limits.h>
#include <linux/dcache.h>
#include <asm/uaccess.h>
#include <linux/path.h>
#include <asm-generic/mman-common.h>
#include "api_proxy.h"
#include "proc_maps_auto_offset.h"
#include "ver_control.h"

#define MY_PATH_MAX_LEN 1024
#pragma pack(push,1)
struct map_entry {
    unsigned long start;
    unsigned long end;
    unsigned char flags[4];
    char path[MY_PATH_MAX_LEN];
};
#pragma pack(pop)

static inline size_t get_proc_map_count(struct pid* proc_pid_struct) {
	ssize_t accurate_offset;
	struct task_struct *task = pid_task(proc_pid_struct, PIDTYPE_PID);
	struct mm_struct *mm = get_task_mm(task);
	size_t count = 0;
	if (g_init_map_count_offset_success == false) {
		mmput(mm);
		return 0;
	}

	if (down_read_mmap_lock(mm) != 0) {
		goto _exit;
	}

	//精确偏移
	accurate_offset = (ssize_t)((size_t)&mm->map_count - (size_t)mm + g_map_count_offset);
	printk_debug(KERN_INFO "mm->map_count accurate_offset:%zd\n", accurate_offset);
	if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t)) {
		mmput(mm);
		return 0;
	}
	count = *(int *)((size_t)mm + (size_t)accurate_offset);

	up_read_mmap_lock(mm);

_exit:mmput(mm);
	return count;
}


static inline int check_proc_map_can_read(struct pid* proc_pid_struct, size_t proc_virt_addr, size_t size) {
	struct task_struct *task = pid_task(proc_pid_struct, PIDTYPE_PID);
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	int res = 0;
	if (!task) { return res; }

	mm = get_task_mm(task);

	if (!mm) { return res; }


	//printk_debug(KERN_EMERG "mm:%p\n", &mm);
	//printk_debug(KERN_EMERG "mm->map_count:%p:%lu\n", &mm->map_count, mm->map_count);
	//printk_debug(KERN_EMERG "mm->mmap_lock:%p\n", &mm->mmap_lock);
	//printk_debug(KERN_EMERG "mm->hiwater_vm:%p:%lu\n", &mm->hiwater_vm, mm->hiwater_vm);
	//printk_debug(KERN_EMERG "mm->total_vm:%p:%lu\n", &mm->total_vm, mm->total_vm);
	//printk_debug(KERN_EMERG "mm->locked_vm:%p:%lu\n", &mm->locked_vm, mm->locked_vm);
	//printk_debug(KERN_EMERG "mm->pinned_vm:%p:%lu\n", &mm->pinned_vm, mm->pinned_vm);

	//printk_debug(KERN_EMERG "mm->task_size:%p:%lu,%lu\n", &mm->task_size, mm->task_size, TASK_SIZE);
	//printk_debug(KERN_EMERG "mm->highest_vm_end:%p:%lu\n", &mm->highest_vm_end, mm->highest_vm_end);
	//printk_debug(KERN_EMERG "mm->pgd:%p:%p\n", &mm->pgd, mm->pgd);


	if (down_read_mmap_lock(mm) != 0) {
		goto _exit;
	}

	vma = find_vma(mm, proc_virt_addr);
	if (vma) {
		if (vma->vm_flags & VM_READ) {
			size_t read_end = proc_virt_addr + size;
			if (read_end <= vma->vm_end) {
				res = 1;
			}
		}
	}
	up_read_mmap_lock(mm);

_exit:mmput(mm);
	return res;
}
static inline int check_proc_map_can_write(struct pid* proc_pid_struct, size_t proc_virt_addr, size_t size) {
	struct task_struct *task = pid_task(proc_pid_struct, PIDTYPE_PID);
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	int res = 0;

	if (!task) { return res; }

	mm = get_task_mm(task);

	if (!mm) { return res; }

	if (down_read_mmap_lock(mm) != 0) {
		mmput(mm);
		return res;
	}

	vma = find_vma(mm, proc_virt_addr);
	if (vma) {
		if (vma->vm_flags & VM_WRITE) {
			size_t read_end = proc_virt_addr + size;
			if (read_end <= vma->vm_end) {
				res = 1;
			}
		}
	}
	up_read_mmap_lock(mm);
	mmput(mm);
	return res;
}


#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(3,10,0)
/* Check if the vma is being used as a stack by this task */
static int vm_is_stack_for_task(struct task_struct *t,
	struct vm_area_struct *vma) {
	return (vma->vm_start <= KSTK_ESP(t) && vma->vm_end >= KSTK_ESP(t));
}

/*
 * Check if the vma is being used as a stack.
 * If is_group is non-zero, check in the entire thread group or else
 * just check in the current task. Returns the pid of the task that
 * the vma is stack for.
 */
static pid_t my_vm_is_stack(struct task_struct *task,
	struct vm_area_struct *vma, int in_group) {
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

static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {

	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;
	
	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);
	if (!mm) {
        ret = -EINVAL;
        goto out;
	}


	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }


	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}

	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		/* We don't show the stack guard page in /proc/maps */
		if (stack_guard_page_start(vma, entry->start))
			entry->start += PAGE_SIZE;
		if (stack_guard_page_end(vma, entry->end))
			entry->end -= PAGE_SIZE;

		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				pid_t tid = my_vm_is_stack(task, vma, 1);
				if (tid != 0) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */

					sprintf(entry->path, "[stack:%d]", tid);
				}
			}

		}

		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}



#endif




#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(3,10,84)
/* Check if the vma is being used as a stack by this task */
static int vm_is_stack_for_task(struct task_struct *t,
	struct vm_area_struct *vma) {
	return (vma->vm_start <= KSTK_ESP(t) && vma->vm_end >= KSTK_ESP(t));
}

/*
 * Check if the vma is being used as a stack.
 * If is_group is non-zero, check in the entire thread group or else
 * just check in the current task. Returns the pid of the task that
 * the vma is stack for.
 */
static pid_t my_vm_is_stack(struct task_struct *task,
	struct vm_area_struct *vma, int in_group) {
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

static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {

	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;
	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);
	if (!mm) {
        ret = -EINVAL;
        goto out;
	}


	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		/* We don't show the stack guard page in /proc/maps */
		if (stack_guard_page_start(vma, entry->start))
			entry->start += PAGE_SIZE;
		if (stack_guard_page_end(vma, entry->end))
			entry->end -= PAGE_SIZE;

		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				pid_t tid = my_vm_is_stack(task, vma, 1);
				if (tid != 0) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */

					sprintf(entry->path, "[stack:%d]", tid);
				}
			}

		}


		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}

#endif


#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(3,18,71)
/* Check if the vma is being used as a stack by this task */
static int vm_is_stack_for_task(struct task_struct *t,
	struct vm_area_struct *vma) {
	return (vma->vm_start <= KSTK_ESP(t) && vma->vm_end >= KSTK_ESP(t));
}


/*
 * Check if the vma is being used as a stack.
 * If is_group is non-zero, check in the entire thread group or else
 * just check in the current task. Returns the task_struct of the task
 * that the vma is stack for. Must be called under rcu_read_lock().
 */
struct task_struct *task_of_stack(struct task_struct *task,
	struct vm_area_struct *vma, bool in_group) {
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
	struct vm_area_struct *vma, bool is_pid) {
	pid_t ret = 0;

	rcu_read_lock();
	task = task_of_stack(task, vma, is_pid);
	if (task) {
		ret = task->pid;
	}
	rcu_read_unlock();

	return ret;
}


static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);
	if (!mm) {
        ret = -EINVAL;
        goto out;
	}

	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				pid_t tid = pid_of_stack(task, vma, 1);
				if (tid != 0) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					if (vma->vm_start <= mm->start_stack &&
						vma->vm_end >= mm->start_stack) {
						snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
					} else {
						snprintf(entry->path, sizeof(entry->path), "[stack:%d]", tid);
					}
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}

#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(3,18,140)
/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}


static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}

	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				if (is_stack(vma)) {
					snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
				}
			}

		}

		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}


#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,4,21)
/* Check if the vma is being used as a stack by this task */
int vma_is_stack_for_task(struct vm_area_struct *vma, struct task_struct *t) {
	return (vma->vm_start <= KSTK_ESP(t) && vma->vm_end >= KSTK_ESP(t));
}

/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct task_struct *task,
	struct vm_area_struct *vma, int is_pid) {
	int stack = 0;

	if (is_pid) {
		stack = vma->vm_start <= vma->vm_mm->start_stack &&
			vma->vm_end >= vma->vm_mm->start_stack;
	} else {
		rcu_read_lock();
		stack = vma_is_stack_for_task(vma, task);
		rcu_read_unlock();
	}
	return stack;
}


static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}

	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;

		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				pid_t tid = is_stack(task, vma, 1);
				if (tid != 0) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					sprintf(entry->path, "[stack:%d]", tid);
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}



#endif




#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,4,78)
/* Check if the vma is being used as a stack by this task */
int vma_is_stack_for_task(struct vm_area_struct *vma, struct task_struct *t) {
	return (vma->vm_start <= KSTK_ESP(t) && vma->vm_end >= KSTK_ESP(t));
}

/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct task_struct *task,
	struct vm_area_struct *vma, int is_pid) {
	int stack = 0;

	if (is_pid) {
		stack = vma->vm_start <= vma->vm_mm->start_stack &&
			vma->vm_end >= vma->vm_mm->start_stack;
	} else {
		rcu_read_lock();
		stack = vma_is_stack_for_task(vma, task);
		rcu_read_unlock();
	}
	return stack;
}


static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}


	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				pid_t tid = is_stack(task, vma, 1);
				if (tid != 0) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					sprintf(entry->path, "[stack:%d]", tid);
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}


#endif




#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,4,153)
/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct task_struct *task,
	struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}


static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}

	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				pid_t tid = is_stack(task, vma);
				if (tid != 0) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}

#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,4,192)
/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct task_struct *task,
	struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}


static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}

	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }


	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}

		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				pid_t tid = is_stack(task, vma);
				if (tid != 0) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}


#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,9,112)
/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}



static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;
	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}

	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区
	
	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}

#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,9,186)
/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}



static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}

	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}


#endif






#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,14,83)


/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}
	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}


#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,14,117)


/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}


	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
				}

			}

		}

		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}



#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,14,141)


/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}
	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}


#endif





#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,19,81)


/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;


	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}
	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;

		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}
#endif






#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,19,113)

/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}
	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}
#endif

#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(5,4,61)

/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}
	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file * vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char *path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}
#endif

#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(5,10,43)

/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct vm_area_struct* vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct* task;
	struct mm_struct* mm;
	struct vm_area_struct* vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;


	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}
	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file* vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char* path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}
#endif

#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(5,15,41)

/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct vm_area_struct* vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct* task;
	struct mm_struct* mm;
	struct vm_area_struct* vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}
	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		struct file* vm_file;
		if (copy_pos + sizeof(*entry) >= end_pos) {
			break;
		}
		memset(entry, 0, sizeof(*entry));
		entry->start = vma->vm_start;
		entry->end   = vma->vm_end;
		entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
		entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
		entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
		entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
		memset(entry->path, 0, sizeof(entry->path));
		vm_file = get_vm_file(vma);
		if (vm_file) {
			char* path;
			memset(path_buf, 0, MY_PATH_MAX_LEN);
			path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
			if (path > 0) {
				strncat(entry->path, path, sizeof(entry->path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void *)copy_pos, entry, sizeof(*entry));
		} else {
			if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
				break;
			}
		}
		copy_pos += sizeof(*entry);
		success_cnt++;
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}
#endif

#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(6,1,75)
#include <linux/mm_inline.h>
struct anon_vma_name * __weak anon_vma_name(struct vm_area_struct* vma) {
	return NULL;
}

/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
static int is_stack(struct vm_area_struct* vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct* task;
	struct mm_struct* mm;
	struct vm_area_struct* vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}
	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}

	{
		VMA_ITERATOR(iter, mm, 0);
		for_each_vma(iter, vma) {
			struct file* vm_file;
			struct anon_vma_name *anon_name = NULL;
			if (copy_pos + sizeof(*entry) >= end_pos) {
				break;
			}
			memset(entry, 0, sizeof(*entry));
			entry->start = vma->vm_start;
			entry->end   = vma->vm_end;
			entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
			entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
			entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
			entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
			memset(entry->path, 0, sizeof(entry->path));
			vm_file = get_vm_file(vma);
			if (vm_file) {
				char* path;
				memset(path_buf, 0, MY_PATH_MAX_LEN);
				path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
				if (path > 0) {
					strncat(entry->path, path, sizeof(entry->path) - 1);
				}
			} else if (!vma->vm_mm) {
				snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
			} else if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else if (is_stack(vma)) {
				/*
				* Thread stack in /proc/PID/task/TID/maps or
				* the main process stack.
				*/

				/* Thread stack in /proc/PID/maps */
				snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
			} else {
				anon_name = anon_vma_name(vma);
				if(anon_name) {
					snprintf(entry->path, sizeof(entry->path), "[anon:%s]", anon_name->name);
				}
			}
			
			if (is_kernel_buf) {
				memcpy((void *)copy_pos, entry, sizeof(*entry));
			} else {
				if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
					break;
				}
			}
			copy_pos += sizeof(*entry);
			success_cnt++;
		}
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}
#endif


#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(6,6,30)
#include <linux/mm_inline.h>
struct anon_vma_name * __weak anon_vma_name(struct vm_area_struct* vma) {
	return NULL;
}

static int get_proc_maps_list(bool is_kernel_buf, struct pid* proc_pid_struct, char* buf, size_t buf_size) {
	struct task_struct* task;
	struct mm_struct* mm;
	struct vm_area_struct* vma;
	char *path_buf = NULL;
    struct map_entry *entry = NULL;
	int success_cnt = 0;
	int ret = 0;
	size_t copy_pos;
	size_t end_pos;

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		ret = -ESRCH;
		goto out;
	}

	mm = get_task_mm(task);

	if (!mm) {
        ret = -EINVAL;
        goto out;
	}
	if (is_kernel_buf) {
		memset(buf, 0, buf_size);
	}
	//else if (clear_user(buf, buf_size)) { return -4; } //清空用户的缓冲区

	path_buf = x_kmalloc(MY_PATH_MAX_LEN, GFP_KERNEL);
    if (!path_buf) {
        ret = -ENOMEM;
        goto out_mm;
    }
    entry = x_kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = -ENOMEM;
        goto out_kpath;
    }

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + buf_size);

	if (down_read_mmap_lock(mm) != 0) {
        ret = -EBUSY;
        goto out_kentry;
	}

	{
		VMA_ITERATOR(iter, mm, 0);
		for_each_vma(iter, vma) {
			struct file* vm_file;
			struct anon_vma_name *anon_name = NULL;
			if (copy_pos + sizeof(*entry) >= end_pos) {
				break;
			}
			memset(entry, 0, sizeof(*entry));
			entry->start = vma->vm_start;
			entry->end   = vma->vm_end;
			entry->flags[0] = (vma->vm_flags & VM_READ)     ? 1 : 0;
			entry->flags[1] = (vma->vm_flags & VM_WRITE)    ? 1 : 0;
			entry->flags[2] = (vma->vm_flags & VM_EXEC)     ? 1 : 0;
			entry->flags[3] = (vma->vm_flags & VM_MAYSHARE) ? 1 : 0;
			memset(entry->path, 0, sizeof(entry->path));
			vm_file = get_vm_file(vma);
			if (vm_file) {
				char* path;
				memset(path_buf, 0, MY_PATH_MAX_LEN);
				path = d_path(&vm_file->f_path, path_buf, MY_PATH_MAX_LEN);
				if (path > 0) {
					strncat(entry->path, path, sizeof(entry->path) - 1);
				}
			} else if (!vma->vm_mm) {
				snprintf(entry->path, sizeof(entry->path), "%s[vdso]", entry->path);
			} else if (vma_is_initial_heap(vma)) {
				snprintf(entry->path, sizeof(entry->path), "%s[heap]", entry->path);
			} else if (vma_is_initial_stack(vma)) {
				snprintf(entry->path, sizeof(entry->path), "%s[stack]", entry->path);
			} else {
				anon_name = anon_vma_name(vma);
				if(anon_name) {
					snprintf(entry->path, sizeof(entry->path), "[anon:%s]", anon_name->name);
				}
			}
				
			if (is_kernel_buf) {
				memcpy((void *)copy_pos, entry, sizeof(*entry));
			} else {
				if (x_copy_to_user((void *)copy_pos, entry, sizeof(*entry))) {
					break;
				}
			}
			copy_pos += sizeof(*entry);
			success_cnt++;
		}
	}
    up_read_mmap_lock(mm);
    ret = success_cnt;

out_kentry:
    kfree(entry);
out_kpath:
    kfree(path_buf);
out_mm:
    mmput(mm);
out:
    return ret;
}
#endif
#endif /* PROC_MAPS_H_ */