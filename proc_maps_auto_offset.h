#ifndef PROC_MAPS_AUTO_OFFSET_H_
#define PROC_MAPS_AUTO_OFFSET_H_
#include "api_proxy.h"
#include "ver_control.h"


#ifndef MM_STRUCT_MMAP_LOCK 
#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(5,10,43)
#define MM_STRUCT_MMAP_LOCK mmap_sem
#endif
#if MY_LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,43)
#define MM_STRUCT_MMAP_LOCK mmap_lock
#endif
#endif



MY_STATIC ssize_t g_mmap_lock_offset_proc_maps = 0;
MY_STATIC bool g_init_mmap_lock_offset_success = false;

MY_STATIC ssize_t g_map_count_offset_proc_maps = 0;
MY_STATIC bool g_init_map_count_offset_success = false;

MY_STATIC ssize_t g_vm_file_offset_proc_maps = 0;
MY_STATIC bool g_init_vm_file_offset_success = false;

MY_STATIC int init_mmap_lock_offset(int proc_self_maps_cnt) {
	int is_find_mmap_lock_offset = 0;
	struct task_struct * mytask = NULL;
	struct mm_struct * mm = NULL;

	mytask = x_get_current();
	if (mytask == NULL) {
		return -EFAULT;
	}
	mm = get_task_mm(mytask);
	if (mm == NULL) {
		return -EFAULT;
	}

	//printk_debug(KERN_EMERG "mm:%p\n", &mm);
	//printk_debug(KERN_EMERG "mm->map_count:%p:%lu\n", &mm->map_count, mm->map_count);
	//printk_debug(KERN_EMERG "mm->mmap_lock:%p\n", &mm->MM_STRUCT_MMAP_LOCK);
	//printk_debug(KERN_EMERG "mm->hiwater_vm:%p:%lu\n", &mm->hiwater_vm, mm->hiwater_vm);
	//printk_debug(KERN_EMERG "mm->total_vm:%p:%lu\n", &mm->total_vm, mm->total_vm);
	//printk_debug(KERN_EMERG "mm->locked_vm:%p:%lu\n", &mm->locked_vm, mm->locked_vm);
	//printk_debug(KERN_EMERG "mm->pinned_vm:%p:%lu\n", &mm->pinned_vm, mm->pinned_vm);

	//printk_debug(KERN_EMERG "mm->task_size:%p:%lu,%lu\n", &mm->task_size, mm->task_size, TASK_SIZE);
	//printk_debug(KERN_EMERG "mm->highest_vm_end:%p:%lu\n", &mm->highest_vm_end, mm->highest_vm_end);
	//printk_debug(KERN_EMERG "mm->pgd:%p:%p\n", &mm->pgd, mm->pgd);


	g_init_mmap_lock_offset_success = true;
	for (g_mmap_lock_offset_proc_maps = -80; g_mmap_lock_offset_proc_maps <= 80; g_mmap_lock_offset_proc_maps += 1) {
		//精确偏移
		char *rp;
		int val;
		ssize_t accurate_offset = (ssize_t)((size_t)&mm->MM_STRUCT_MMAP_LOCK - (size_t)mm + g_mmap_lock_offset_proc_maps);
		if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t)) {
			mmput(mm);
			return -EFAULT;
		}
		rp = (char*)((size_t)mm + (size_t)accurate_offset);
		val = *(int*)(rp);
		printk_debug(KERN_EMERG "init_mmap_lock_offset %zd:%zd:%p:%d\n", g_mmap_lock_offset_proc_maps, accurate_offset, rp, val);

		if (val == proc_self_maps_cnt) {
			printk_debug(KERN_EMERG "val == proc_self_maps_cnt %zd:%zd:%p:%d\n", g_mmap_lock_offset_proc_maps, accurate_offset, rp, val);
			//找到了
			g_mmap_lock_offset_proc_maps += sizeof(val); //跳过自身
			g_mmap_lock_offset_proc_maps += sizeof(int); //再跳过另一个int变量 spinlock_t page_table_lock
			is_find_mmap_lock_offset = 1;
			break;
		}

	}


	if (!is_find_mmap_lock_offset) {
		g_init_mmap_lock_offset_success = false;
		mmput(mm);
		printk_debug(KERN_INFO "find mmap_lock offset failed\n");
		return -ESPIPE;
	}
	mmput(mm);
	printk_debug(KERN_INFO "found g_mmap_lock_offset_proc_maps:%zu\n", g_mmap_lock_offset_proc_maps);
	return 0;
}

MY_STATIC inline int down_read_mmap_lock(struct mm_struct *mm) {
	ssize_t accurate_offset;
	struct rw_semaphore *sem;
	if (g_init_mmap_lock_offset_success == false) {
		return -ENOENT;
	}

	//精确偏移
	accurate_offset = (ssize_t)((size_t)&mm->MM_STRUCT_MMAP_LOCK - (size_t)mm + g_mmap_lock_offset_proc_maps);
	printk_debug(KERN_INFO "down_read_mmap_lock accurate_offset:%zd\n", accurate_offset);
	if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t)) {
		return -ERANGE;
	}
	sem = (struct rw_semaphore *)((size_t)mm + (size_t)accurate_offset);
	down_read(sem);
	return 0;
}
MY_STATIC inline int up_read_mmap_lock(struct mm_struct *mm) {
	ssize_t accurate_offset;
	struct rw_semaphore *sem;
	if (g_init_mmap_lock_offset_success == false) {
		return -ENOENT;
	}
	//精确偏移
	accurate_offset = (ssize_t)((size_t)&mm->MM_STRUCT_MMAP_LOCK - (size_t)mm + g_mmap_lock_offset_proc_maps);
	printk_debug(KERN_INFO "accurate_offset:%zd\n", accurate_offset);
	if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t)) {
		return -ERANGE;
	}
	sem = (struct rw_semaphore *)((size_t)mm + (size_t)accurate_offset);

	up_read(sem);
	return 0;
}

MY_STATIC int init_map_count_offset(int proc_self_maps_cnt) {
	int is_find_map_count_offset = 0;
	struct task_struct * mytask = NULL;
	struct mm_struct * mm = NULL;

	mytask = x_get_current();
	if (mytask == NULL) {
		return -EFAULT;
	}
	mm = get_task_mm(mytask);
	if (mm == NULL) {
		return -EFAULT;
	}

	g_init_map_count_offset_success = true;
	for (g_map_count_offset_proc_maps = -40; g_map_count_offset_proc_maps <= 40; g_map_count_offset_proc_maps += 1) {
		//精确偏移
		char *rp;
		int val;
		ssize_t accurate_offset = (ssize_t)((size_t)&mm->map_count - (size_t)mm + g_map_count_offset_proc_maps);
		if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t)) {
			mmput(mm);
			return -EFAULT;
		}
		rp = (char*)((size_t)mm + (size_t)accurate_offset);
		val = *(int*)(rp);
		printk_debug(KERN_EMERG "init_map_count_offset %zd:%zd:%p:%d\n", g_map_count_offset_proc_maps, accurate_offset, rp, val);

		if (val == proc_self_maps_cnt) {
			//找到了
			printk_debug(KERN_EMERG "val == proc_self_maps_cnt %zd:%zd:%p:%d\n", g_map_count_offset_proc_maps, accurate_offset, rp, val);
			is_find_map_count_offset = 1;
			break;
		}
	}


	if (!is_find_map_count_offset) {
		g_init_map_count_offset_success = false;
		printk_debug(KERN_INFO "find map_count offset failed\n");
		mmput(mm);
		return -ESPIPE;
	}

	mmput(mm);
	printk_debug(KERN_INFO "g_map_count_offset_proc_maps:%zu\n", g_map_count_offset_proc_maps);
	return 0;
}

MY_STATIC int init_vm_file_offset(void) {
	int is_find_vm_file_offset = 0;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	struct task_struct * mytask = x_get_current();

	if (mytask == NULL) {
		return -EFAULT;
	}
	mm = get_task_mm(mytask);
	if (!mm) {
		return -3;
	}

	if (down_read_mmap_lock(mm) != 0) {
		mmput(mm);
		return -4;
	}

	g_init_vm_file_offset_success = true;
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		if (is_find_vm_file_offset == 1) {
			//已经找到了
			break;
		}
		for (g_vm_file_offset_proc_maps = -80; g_vm_file_offset_proc_maps <= 80; g_vm_file_offset_proc_maps += 1) {
			//精确偏移
			char *rp;
			size_t addr_val1;
			size_t addr_val2;
			unsigned long vm_pgoff;
			ssize_t accurate_offset = (ssize_t)((size_t)&vma->vm_file - (size_t)vma + g_vm_file_offset_proc_maps);
			//这里故意屏蔽，因为vm_file已经接近vm_area_struct结构体尾部了
			/*if (accurate_offset >= sizeof(struct vm_area_struct) - sizeof(struct file *))
			{
				mmput(mm);
				return -EFAULT;
			}*/
			rp = (char*)((size_t)vma + (size_t)accurate_offset);
			addr_val1 = *(size_t*)(rp);
			rp += (size_t)sizeof(void*);
			addr_val2 = *(size_t*)(rp);
			printk_debug(KERN_EMERG "init_vm_file_offset %zd:%zd:%p:%p\n", g_vm_file_offset_proc_maps, accurate_offset, rp, addr_val1);
			if (addr_val1 > 0 && addr_val2 > 0 && addr_val1 == addr_val2) //struct list_head anon_vma_chain;里面两个值一样
			{
				int vm_pgoff_offset = 0;
				int found_vm_pgoff = 0;

				printk_debug(KERN_EMERG "addr_val1 == addr_val2 %zd:%zd:%p:%p\n", g_vm_file_offset_proc_maps, accurate_offset, rp, addr_val1);
				rp += (size_t)sizeof(void*);
				for (; vm_pgoff_offset < 8 * 5; vm_pgoff_offset += 4) {
					vm_pgoff = *(unsigned long*)(rp);
					if (vm_pgoff > 0 && vm_pgoff < 1000/*这个值是vm_pgoff我见过的最大值吧，如果最大值比1000还有大再改大*/) {
						found_vm_pgoff = 1;
						break;
					}
					rp += 4;
				}
				if (found_vm_pgoff) {
					rp += (size_t)sizeof(unsigned long); //vm_pgoff
					rp += (size_t)sizeof(struct file *);

					addr_val1 = *(size_t*)(rp); //void * vm_private_data;
					rp += (size_t)sizeof(void*);
					addr_val2 = *(size_t*)(rp); //atomic_long_t swap_readahead_info;

					if (addr_val1 == 0 && addr_val2 == 0) {
						g_vm_file_offset_proc_maps += sizeof(void*) * 2; //struct list_head anon_vma_chain;
						g_vm_file_offset_proc_maps += vm_pgoff_offset;
						g_vm_file_offset_proc_maps += sizeof(unsigned long); //vm_pgoff
						printk_debug(KERN_EMERG "addr_val1 == addr_val2 == 0 %zd:%d\n", g_vm_file_offset_proc_maps, vm_pgoff_offset);
						//找到了
						is_find_vm_file_offset = 1;
						break;
					}

				}


			}
		}
	}
	up_read_mmap_lock(mm);
	mmput(mm);

	if (!is_find_vm_file_offset) {
		g_init_vm_file_offset_success = false;
		printk_debug(KERN_INFO "find vm_file offset failed\n");
		return -ESPIPE;
	}
	return 0;
}

MY_STATIC inline struct file * get_vm_file(struct vm_area_struct *vma) {
	struct file * vm_file;
	ssize_t accurate_offset;
	if (g_init_vm_file_offset_success == false) {
		if (init_vm_file_offset() != 0) {
			return NULL;
		}
	}

	//精确偏移
	accurate_offset = (ssize_t)((size_t)&vma->vm_file - (size_t)vma + g_vm_file_offset_proc_maps);
	printk_debug(KERN_INFO "get_vm_file accurate_offset:%zd\n", accurate_offset);
	//这里故意屏蔽，因为vm_file已经接近vm_area_struct结构体尾部了
	//if (accurate_offset >= sizeof(struct vm_area_struct) - sizeof(struct file *))
	//{
	//	return NULL;
	//}
	vm_file = (struct file*) *(size_t*)((size_t)vma + (size_t)accurate_offset);
	return vm_file;
}
#endif /* PROC_MAPS_AUTO_OFFSET_H_ */