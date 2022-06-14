#ifndef PROC_MAPS_H_
#define PROC_MAPS_H_

//声明
//////////////////////////////////////////////////////////////////////////
#include <linux/pid.h>
#include <linux/types.h>
#include <linux/mm_types.h>

#ifndef MM_STRUCT_MMAP_LOCK 
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(5,10,43)
#define MM_STRUCT_MMAP_LOCK mmap_sem
#endif
#if MY_LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,43)
#define MM_STRUCT_MMAP_LOCK mmap_lock
#endif
#endif

MY_STATIC inline size_t get_proc_map_count(struct pid* proc_pid_struct);
MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass);


//实现
//////////////////////////////////////////////////////////////////////////
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/security.h>
#include <linux/slab.h> //kmalloc与kfree
#include <linux/sched.h>
#include <linux/limits.h>
#include <linux/dcache.h>
#include <asm/uaccess.h>
#include <linux/path.h>
#include <asm-generic/mman-common.h>
#include "api_proxy.h"
#include "ver_control.h"

#define MY_PATH_MAX_LEN 512


MY_STATIC inline size_t get_proc_map_count(struct pid* proc_pid_struct) {
	struct mm_struct *mm;
	size_t count = 0;
	struct task_struct *task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return 0;
	}

	mm = get_task_mm(task);

	if (!mm) {
		return 0;
	}
	down_read(&mm->MM_STRUCT_MMAP_LOCK);
	count = mm->map_count;
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return count;
}


MY_STATIC inline int check_proc_map_can_read(struct pid* proc_pid_struct, size_t proc_virt_addr, size_t size) {
	struct task_struct *task = pid_task(proc_pid_struct, PIDTYPE_PID);
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	int res = 0;
	if (!task) { return res; }

	mm = get_task_mm(task);

	if (!mm) { return res; }

	down_read(&mm->MM_STRUCT_MMAP_LOCK);

	vma = find_vma(mm, proc_virt_addr);
	if (vma) {
		if (vma->vm_flags & VM_READ) {
			size_t read_end = proc_virt_addr + size;
			if (read_end <= vma->vm_end) {
				res = 1;
			}
		}
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);
	return res;
}
MY_STATIC inline int check_proc_map_can_write(struct pid* proc_pid_struct, size_t proc_virt_addr, size_t size) {
	struct task_struct *task = pid_task(proc_pid_struct, PIDTYPE_PID);
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	int res = 0;

	if (!task) { return res; }

	mm = get_task_mm(task);

	if (!mm) { return res; }

	down_read(&mm->MM_STRUCT_MMAP_LOCK);

	vma = find_vma(mm, proc_virt_addr);
	if (vma) {
		if (vma->vm_flags & VM_WRITE) {
			size_t read_end = proc_virt_addr + size;
			if (read_end <= vma->vm_end) {
				res = 1;
			}
		}
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);
	return res;
}


#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(3,10,0)
/* Check if the vma is being used as a stack by this task */
MY_STATIC int vm_is_stack_for_task(struct task_struct *t,
	struct vm_area_struct *vma) {
	return (vma->vm_start <= KSTK_ESP(t) && vma->vm_end >= KSTK_ESP(t));
}

/*
 * Check if the vma is being used as a stack.
 * If is_group is non-zero, check in the entire thread group or else
 * just check in the current task. Returns the pid of the task that
 * the vma is stack for.
 */
MY_STATIC pid_t vm_is_stack(struct task_struct *task,
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

MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {

	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;


	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);
	if (!mm) {
		return -3;
	}


	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //Çå¿ÕÓÃ»§µÄ»º³åÇø

	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->MM_STRUCT_MMAP_LOCK);
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];

		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;

		/* We don't show the stack guard page in /proc/maps */
		start = vma->vm_start;
		if (stack_guard_page_start(vma, start))
			start += PAGE_SIZE;
		end = vma->vm_end;
		if (stack_guard_page_end(vma, end))
			end -= PAGE_SIZE;

		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';



		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				pid_t tid = vm_is_stack(task, vma, 1);
				if (tid != 0) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */

					sprintf(new_path, "[stack:%d]", tid);
				}
			}

		}

		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;

		} else {
			//ÄÚºË¿Õ¼ä->ÓÃ»§¿Õ¼ä½»»»Êý¾Ý
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}



#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(3,10,84)
/* Check if the vma is being used as a stack by this task */
MY_STATIC int vm_is_stack_for_task(struct task_struct *t,
	struct vm_area_struct *vma) {
	return (vma->vm_start <= KSTK_ESP(t) && vma->vm_end >= KSTK_ESP(t));
}

/*
 * Check if the vma is being used as a stack.
 * If is_group is non-zero, check in the entire thread group or else
 * just check in the current task. Returns the pid of the task that
 * the vma is stack for.
 */
MY_STATIC pid_t my_vm_is_stack(struct task_struct *task,
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

MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {

	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;

	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);
	if (!mm) {
		return -3;
	}


	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //Çå¿ÕÓÃ»§µÄ»º³åÇø


	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->MM_STRUCT_MMAP_LOCK);
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];

		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;

		/* We don't show the stack guard page in /proc/maps */
		start = vma->vm_start;
		if (stack_guard_page_start(vma, start))
			start += PAGE_SIZE;
		end = vma->vm_end;
		if (stack_guard_page_end(vma, end))
			end -= PAGE_SIZE;


		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';


		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				pid_t tid = my_vm_is_stack(task, vma, 1);
				if (tid != 0) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */

					sprintf(new_path, "[stack:%d]", tid);
				}
			}

		}


		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//ÄÚºË¿Õ¼ä->ÓÃ»§¿Õ¼ä½»»»Êý¾Ý
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}

#endif


#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(3,18,71)
/* Check if the vma is being used as a stack by this task */
MY_STATIC int vm_is_stack_for_task(struct task_struct *t,
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


MY_STATIC pid_t pid_of_stack(struct task_struct *task,
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


MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;

	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);
	if (!mm) {
		return -3;
	}

	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //Çå¿ÕÓÃ»§µÄ»º³åÇø


	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->MM_STRUCT_MMAP_LOCK);
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];

		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;


		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';


		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
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
						if ((sizeof(new_path) - strlen(new_path) - 8) >= 0) {
							strcat(new_path, "[stack]");
						}
					} else {
						snprintf(new_path, sizeof(new_path), "[stack:%d]", tid);
					}
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//ÄÚºË¿Õ¼ä->ÓÃ»§¿Õ¼ä½»»»Êý¾Ý
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}

#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(3,18,140)
/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
MY_STATIC int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}


MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;

	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);

	if (!mm) {
		return -3;
	}

	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //Çå¿ÕÓÃ»§µÄ»º³åÇø


	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->MM_STRUCT_MMAP_LOCK);
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];

		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;


		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';


		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				if (is_stack(vma)) {
					if ((sizeof(new_path) - strlen(new_path) - 8) >= 0) {
						strcat(new_path, "[stack]");
					}
				}
			}

		}

		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//ÄÚºË¿Õ¼ä->ÓÃ»§¿Õ¼ä½»»»Êý¾Ý
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}


#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,4,21)
/* Check if the vma is being used as a stack by this task */
MY_STATIC int vma_is_stack_for_task(struct vm_area_struct *vma, struct task_struct *t) {
	return (vma->vm_start <= KSTK_ESP(t) && vma->vm_end >= KSTK_ESP(t));
}

/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
MY_STATIC int is_stack(struct task_struct *task,
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


MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;

	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);

	if (!mm) {
		return -3;
	}

	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //Çå¿ÕÓÃ»§µÄ»º³åÇø

	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	if (down_read_mmap_sem(mm) != 0) {
		mmput(mm);
		return -4;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];


		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;



		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';


		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				pid_t tid = is_stack(task, vma, 1);
				if (tid != 0) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					sprintf(new_path, "[stack:%d]", tid);
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//ÄÚºË¿Õ¼ä->ÓÃ»§¿Õ¼ä½»»»Êý¾Ý
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
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
MY_STATIC int is_stack(struct task_struct *task,
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


MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;

	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);

	if (!mm) {
		return -3;
	}


	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //Çå¿ÕÓÃ»§µÄ»º³åÇø


	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->MM_STRUCT_MMAP_LOCK);
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];
		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;



		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				pid_t tid = is_stack(task, vma, 1);
				if (tid != 0) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					sprintf(new_path, "[stack:%d]", tid);
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//ÄÚºË¿Õ¼ä->ÓÃ»§¿Õ¼ä½»»»Êý¾Ý
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}


#endif




#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,4,153)
/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
MY_STATIC int is_stack(struct task_struct *task,
	struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}


MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;


	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);

	if (!mm) {
		return -3;
	}

	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //Çå¿ÕÓÃ»§µÄ»º³åÇø

	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->MM_STRUCT_MMAP_LOCK);
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];

		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;


		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';


		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				pid_t tid = is_stack(task, vma);
				if (tid != 0) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					if ((sizeof(new_path) - strlen(new_path) - 8) >= 0) {
						strcat(new_path, "[stack]");
					}
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//ÄÚºË¿Õ¼ä->ÓÃ»§¿Õ¼ä½»»»Êý¾Ý
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}

#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,4,192)
/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
MY_STATIC int is_stack(struct task_struct *task,
	struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}


MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;


	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);

	if (!mm) {
		return -3;
	}

	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //Çå¿ÕÓÃ»§µÄ»º³åÇø


	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->MM_STRUCT_MMAP_LOCK);
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];

		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;



		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';


		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}

		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				pid_t tid = is_stack(task, vma);
				if (tid != 0) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					if ((sizeof(new_path) - strlen(new_path) - 8) >= 0) {
						strcat(new_path, "[stack]");
					}
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//ÄÚºË¿Õ¼ä->ÓÃ»§¿Õ¼ä½»»»Êý¾Ý
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}


#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,9,112)
/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
MY_STATIC int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}



MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;


	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);

	if (!mm) {
		return -3;
	}

	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //Çå¿ÕÓÃ»§µÄ»º³åÇø

	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->MM_STRUCT_MMAP_LOCK);
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];

		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;



		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';


		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					if ((sizeof(new_path) - strlen(new_path) - 8) >= 0) {
						strcat(new_path, "[stack]");
					}
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//ÄÚºË¿Õ¼ä->ÓÃ»§¿Õ¼ä½»»»Êý¾Ý
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}

#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,9,186)
/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
MY_STATIC int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}



MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;


	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);

	if (!mm) {
		return -3;
	}

	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //Çå¿ÕÓÃ»§µÄ»º³åÇø


	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->MM_STRUCT_MMAP_LOCK);
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];

		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;


		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';


		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					if ((sizeof(new_path) - strlen(new_path) - 8) >= 0) {
						strcat(new_path, "[stack]");
					}
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//ÄÚºË¿Õ¼ä->ÓÃ»§¿Õ¼ä½»»»Êý¾Ý
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}


#endif






#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,14,83)


/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
MY_STATIC int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;


	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);

	if (!mm) {
		return -3;
	}
	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //Çå¿ÕÓÃ»§µÄ»º³åÇø

	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	if (down_read_mmap_sem(mm) != 0) {
		mmput(mm);
		return -4;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];

		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;




		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					if ((sizeof(new_path) - strlen(new_path) - 8) >= 0) {
						strcat(new_path, "[stack]");
					}
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//ÄÚºË¿Õ¼ä->ÓÃ»§¿Õ¼ä½»»»Êý¾Ý
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}


#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,14,117)


/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
MY_STATIC int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;


	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);

	if (!mm) {
		return -3;
	}


	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //Çå¿ÕÓÃ»§µÄ»º³åÇø

	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->MM_STRUCT_MMAP_LOCK);
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];

		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;



		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';

		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					if ((sizeof(new_path) - strlen(new_path) - 8) >= 0) {
						strcat(new_path, "[stack]");
					}
				}

			}

		}

		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//ÄÚºË¿Õ¼ä->ÓÃ»§¿Õ¼ä½»»»Êý¾Ý
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;
		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}



#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,14,141)


/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
MY_STATIC int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;


	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);

	if (!mm) {
		return -3;
	}
	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //清空用户的缓冲区

	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	if (down_read_mmap_sem(mm) != 0) {
		mmput(mm);
		return -4;
	}
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];

		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;




		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';


		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					if ((sizeof(new_path) - strlen(new_path) - 8) >= 0) {
						strcat(new_path, "[stack]");
					}
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//ÄÚºË¿Õ¼ä->ÓÃ»§¿Õ¼ä½»»»Êý¾Ý
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}


#endif





#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,19,113)

/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
MY_STATIC int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;


	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);

	if (!mm) {
		return -3;
	}
	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //Çå¿ÕÓÃ»§µÄ»º³åÇø

	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->MM_STRUCT_MMAP_LOCK);
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];

		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;



		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';


		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					if ((sizeof(new_path) - strlen(new_path) - 8) >= 0) {
						strcat(new_path, "[stack]");
					}
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//ÄÚºË¿Õ¼ä->ÓÃ»§¿Õ¼ä½»»»Êý¾Ý
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}
#endif




#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(4,19,81)

/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
MY_STATIC int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;


	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);

	if (!mm) {
		return -3;
	}
	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //清空用户的缓冲区

	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->MM_STRUCT_MMAP_LOCK);
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];

		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;



		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';


		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					if ((sizeof(new_path) - strlen(new_path) - 8) >= 0) {
						strcat(new_path, "[stack]");
					}
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//内核空间->用户空间交换数据
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}
#endif

#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(5,4,61)

/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
MY_STATIC int is_stack(struct vm_area_struct *vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int *have_pass) {
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;


	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);

	if (!mm) {
		return -3;
	}
	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //清空用户的缓冲区

	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->MM_STRUCT_MMAP_LOCK);

	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];

		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;



		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';


		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char *path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					if ((sizeof(new_path) - strlen(new_path) - 8) >= 0) {
						strcat(new_path, "[stack]");
					}
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//内核空间->用户空间交换数据
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}
#endif



#if MY_LINUX_VERSION_CODE == KERNEL_VERSION(5,10,43)

/*
 * Indicate if the VMA is a stack for the given task; for
 * /proc/PID/maps that is the stack of the main task.
 */
MY_STATIC int is_stack(struct vm_area_struct* vma) {
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

MY_STATIC int get_proc_maps_list(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, bool is_kernel_buf, int* have_pass) {
	struct task_struct* task;
	struct mm_struct* mm;
	struct vm_area_struct* vma;
	char new_path[MY_PATH_MAX_LEN];
	char path_buf[MY_PATH_MAX_LEN];
	int success = 0;
	size_t copy_pos;
	size_t end_pos;


	if (max_path_length <= 0) {
		return -1;
	}

	task = pid_task(proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		return -2;
	}

	mm = get_task_mm(task);

	if (!mm) {
		return -3;
	}
	if (is_kernel_buf) {
		memset(lpBuf, 0, buf_size);
	}
	//else if (clear_user(lpBuf, buf_size)) { return -4; } //清空用户的缓冲区

	copy_pos = (size_t)lpBuf;
	end_pos = (size_t)((size_t)lpBuf + buf_size);

	down_read(&mm->MM_STRUCT_MMAP_LOCK);

	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		unsigned long start, end;
		unsigned char flags[4];

		if (copy_pos >= end_pos) {
			if (have_pass) {
				*have_pass = 1;
			}
			break;
		}
		start = vma->vm_start;
		end = vma->vm_end;



		flags[0] = vma->vm_flags & VM_READ ? '\x01' : '\x00';
		flags[1] = vma->vm_flags & VM_WRITE ? '\x01' : '\x00';
		flags[2] = vma->vm_flags & VM_EXEC ? '\x01' : '\x00';
		flags[3] = vma->vm_flags & VM_MAYSHARE ? '\x01' : '\x00';


		memset(new_path, 0, sizeof(new_path));
		if (vma->vm_file) {
			char* path;
			memset(path_buf, 0, sizeof(path_buf));
			path = d_path(&vma->vm_file->f_path, path_buf, sizeof(path_buf));
			if (path > 0) {
				strncat(new_path, path, sizeof(new_path) - 1);
			}
		} else if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
			if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
				strcat(new_path, "[vdso]");
			}
		} else {
			if (vma->vm_start <= mm->brk &&
				vma->vm_end >= mm->start_brk) {
				if ((sizeof(new_path) - strlen(new_path) - 7) >= 0) {
					strcat(new_path, "[heap]");
				}
			} else {
				if (is_stack(vma)) {
					/*
					 * Thread stack in /proc/PID/task/TID/maps or
					 * the main process stack.
					 */

					 /* Thread stack in /proc/PID/maps */
					if ((sizeof(new_path) - strlen(new_path) - 8) >= 0) {
						strcat(new_path, "[stack]");
					}
				}

			}

		}
		if (is_kernel_buf) {
			memcpy((void*)copy_pos, &start, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &end, 8);
			copy_pos += 8;
			memcpy((void*)copy_pos, &flags, 4);
			copy_pos += 4;
			memcpy((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1);
			copy_pos += max_path_length;
		} else {
			//内核空间->用户空间交换数据
			if (!!copy_to_user((void*)copy_pos, &start, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &end, 8)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 8;

			if (!!copy_to_user((void*)copy_pos, &flags, 4)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += 4;

			if (!!copy_to_user((void*)copy_pos, &new_path, max_path_length > MY_PATH_MAX_LEN ? MY_PATH_MAX_LEN : max_path_length - 1)) {
				if (have_pass) {
					*have_pass = 1;
				}
				break;
			}
			copy_pos += max_path_length;

		}
		success++;
	}
	up_read(&mm->MM_STRUCT_MMAP_LOCK);
	mmput(mm);

	return success;
}
#endif

#endif /* PROC_MAPS_H_ */