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

static ssize_t g_mmap_lock_offset = 0;
static bool g_init_mmap_lock_offset_success = false;

static ssize_t g_map_count_offset = 0;
static bool g_init_map_count_offset_success = false;

static ssize_t g_vm_file_offset = 0;
static bool g_init_vm_file_offset_success = false;

static int get_mytask_maps_cnt(void) {
	struct task_struct * mytask = x_get_current();
	struct mm_struct * mm = get_task_mm(mytask);
	struct vm_area_struct* vma;
	int cnt = 0;
	#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(6,1,0)
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		cnt++;
	}
	#else
	{
		VMA_ITERATOR(iter, mm, 0);
		for_each_vma(iter, vma) {
			cnt++;
		}
	}
	#endif
	mmput(mm);
	return cnt;
}


static int init_mmap_lock_offset(void) {
	int is_found_mmap_lock_offset = 0;
	struct task_struct * mytask = x_get_current();
	struct mm_struct * mm = get_task_mm(mytask);
	int maps_cnt = get_mytask_maps_cnt();
	if(g_init_mmap_lock_offset_success) {
		mmput(mm);
		return 0;
	}
	printk_debug(KERN_EMERG "init_mmap_lock_offset maps_cnt:%d, mm->map_count:%p:%d\n", maps_cnt, &mm->map_count, (int)mm->map_count);
	g_init_mmap_lock_offset_success = true;
	for (g_mmap_lock_offset = -80; g_mmap_lock_offset <= 80; g_mmap_lock_offset += 1) {
		char *rp;
		int val;
		ssize_t accurate_offset = (ssize_t)((size_t)&mm->MM_STRUCT_MMAP_LOCK - (size_t)mm + g_mmap_lock_offset);
		if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t)) {
			mmput(mm);
			return -EFAULT;
		}
		rp = (char*)((size_t)mm + (size_t)accurate_offset);
		val = *(int*)(rp);
		printk_debug(KERN_EMERG "init_mmap_lock_offset %zd:%zd:%p:%d\n", g_mmap_lock_offset, accurate_offset, rp, val);

		if (val == maps_cnt) {
			printk_debug(KERN_EMERG "val == maps_cnt %zd:%zd:%p:%d\n", g_mmap_lock_offset, accurate_offset, rp, val);
			g_mmap_lock_offset += sizeof(val);
			g_mmap_lock_offset += sizeof(int);
			is_found_mmap_lock_offset = 1;
			break;
		}

	}


	if (!is_found_mmap_lock_offset) {
		g_init_mmap_lock_offset_success = false;
		mmput(mm);
		printk_debug(KERN_INFO "find mmap_lock offset failed\n");
		return -ESPIPE;
	}
	mmput(mm);
	printk_debug(KERN_INFO "found g_mmap_lock_offset:%zu\n", g_mmap_lock_offset);
	return 0;
}

static inline int down_read_mmap_lock(struct mm_struct *mm) {
	ssize_t accurate_offset;
	struct rw_semaphore *sem;
	if (g_init_mmap_lock_offset_success == false) {
		return -ENOENT;
	}

	accurate_offset = (ssize_t)((size_t)&mm->MM_STRUCT_MMAP_LOCK - (size_t)mm + g_mmap_lock_offset);
	printk_debug(KERN_INFO "down_read_mmap_lock accurate_offset:%zd\n", accurate_offset);
	if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t)) {
		return -ERANGE;
	}
	sem = (struct rw_semaphore *)((size_t)mm + (size_t)accurate_offset);
	down_read(sem);
	return 0;
}

static inline int up_read_mmap_lock(struct mm_struct *mm) {
	ssize_t accurate_offset;
	struct rw_semaphore *sem;
	if (g_init_mmap_lock_offset_success == false) {
		return -ENOENT;
	}
	accurate_offset = (ssize_t)((size_t)&mm->MM_STRUCT_MMAP_LOCK - (size_t)mm + g_mmap_lock_offset);
	printk_debug(KERN_INFO "accurate_offset:%zd\n", accurate_offset);
	if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t)) {
		return -ERANGE;
	}
	sem = (struct rw_semaphore *)((size_t)mm + (size_t)accurate_offset);

	up_read(sem);
	return 0;
}

static int init_map_count_offset(void) {
	int is_found_map_count_offset = 0;
	struct task_struct * mytask = x_get_current();
	struct mm_struct * mm = get_task_mm(mytask);
	int maps_cnt = get_mytask_maps_cnt();
	if(g_init_map_count_offset_success) {
		mmput(mm);
		return 0;
	}
	printk_debug(KERN_EMERG "init_map_count_offset maps_cnt:%d, mm->map_count:%p:%d\n", maps_cnt, &mm->map_count, (int)mm->map_count);
	
	g_init_map_count_offset_success = true;
	for (g_map_count_offset = -40; g_map_count_offset <= 40; g_map_count_offset += 1) {
		char *rp;
		int val;
		ssize_t accurate_offset = (ssize_t)((size_t)&mm->map_count - (size_t)mm + g_map_count_offset);
		if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t)) {
			mmput(mm);
			return -EFAULT;
		}
		rp = (char*)((size_t)mm + (size_t)accurate_offset);
		val = *(int*)(rp);
		printk_debug(KERN_EMERG "init_map_count_offset %zd:%zd:%p:%d\n", g_map_count_offset, accurate_offset, rp, val);

		if (val == maps_cnt) {
			printk_debug(KERN_EMERG "val == maps_cnt %zd:%zd:%p:%d\n", g_map_count_offset, accurate_offset, rp, val);
			is_found_map_count_offset = 1;
			break;
		}
	}


	if (!is_found_map_count_offset) {
		g_init_map_count_offset_success = false;
		printk_debug(KERN_INFO "find map_count offset failed\n");
		mmput(mm);
		return -ESPIPE;
	}

	mmput(mm);
	printk_debug(KERN_INFO "g_map_count_offset:%zu\n", g_map_count_offset);
	return 0;
}

#if MY_LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,75)
static int init_vm_file_offset(void) {
	int is_found_vm_file_offset = 0;
	struct vm_area_struct *vma;
	struct task_struct * mytask = x_get_current();
	struct mm_struct *mm = get_task_mm(mytask);
	if(g_init_vm_file_offset_success) {
		mmput(mm);
		return 0;
	}
	if (down_read_mmap_lock(mm) != 0) {
		mmput(mm);
		return -EFAULT;
	}

	g_init_vm_file_offset_success = false;
	{
		VMA_ITERATOR(iter, mm, 0);
		for_each_vma(iter, vma) {
			if (is_found_vm_file_offset == 1) {
				break;
			}
			for (g_vm_file_offset = -80; g_vm_file_offset <= 80; g_vm_file_offset += 1) {
				char *rp;
				size_t addr_val1;
				size_t addr_val2;
				unsigned long vm_pgoff;
				ssize_t accurate_offset = (ssize_t)((size_t)&vma->vm_file - (size_t)vma + g_vm_file_offset);
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
				printk_debug(KERN_EMERG "init_vm_file_offset %zd:%zd:%p:%zu\n", g_vm_file_offset, accurate_offset, rp, addr_val1);
				if (addr_val1 > 0 && addr_val2 > 0 && addr_val1 == addr_val2) //struct list_head anon_vma_chain;里面两个值一样
				{
					int vm_pgoff_offset = 0;
					int found_vm_pgoff = 0;

					printk_debug(KERN_EMERG "init_vm_file_offset addr_val1 == addr_val2 %zd:%zd:%p:%zu\n", g_vm_file_offset, accurate_offset, rp, addr_val1);
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
						rp += (size_t)sizeof(unsigned long);
						rp += (size_t)sizeof(struct file *);

						addr_val1 = *(size_t*)(rp);
						rp += (size_t)sizeof(void*);
						addr_val2 = *(size_t*)(rp);

						if (addr_val1 == 0 && addr_val2 == 0) {
							g_vm_file_offset += sizeof(void*) * 2;
							g_vm_file_offset += vm_pgoff_offset;
							g_vm_file_offset += sizeof(unsigned long);
							printk_debug(KERN_EMERG "init_vm_file_offset ok, addr_val1 == addr_val2 == 0 %zd:%d\n", g_vm_file_offset, vm_pgoff_offset);
							is_found_vm_file_offset = 1;
							break;
						}

					}


				}
			}
		}
	}

	up_read_mmap_lock(mm);
	mmput(mm);

	if (!is_found_vm_file_offset) {
		printk_debug(KERN_INFO "find vm_file offset failed\n");
		return -ESPIPE;
	}
	g_init_vm_file_offset_success = true;

	return 0;
}
#else

static int init_vm_file_offset(void) {
	int is_found_vm_file_offset = 0;
	struct vm_area_struct *vma;
	struct task_struct * mytask = x_get_current();
	struct mm_struct *mm = get_task_mm(mytask);
	if(g_init_vm_file_offset_success) {
		mmput(mm);
		return 0;
	}
	if (down_read_mmap_lock(mm) != 0) {
		mmput(mm);
		return -EFAULT;
	}

	g_init_vm_file_offset_success = false;
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		if (is_found_vm_file_offset == 1) {
			//已经找到了
			break;
		}
		for (g_vm_file_offset = -80; g_vm_file_offset <= 80; g_vm_file_offset += 1) {
			char *rp;
			size_t addr_val1;
			size_t addr_val2;
			unsigned long vm_pgoff;
			ssize_t accurate_offset = (ssize_t)((size_t)&vma->vm_file - (size_t)vma + g_vm_file_offset);
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
			printk_debug(KERN_EMERG "init_vm_file_offset %zd:%zd:%p:%zu\n", g_vm_file_offset, accurate_offset, rp, addr_val1);
			if (addr_val1 > 0 && addr_val2 > 0 && addr_val1 == addr_val2) //struct list_head anon_vma_chain;里面两个值一样
			{
				int vm_pgoff_offset = 0;
				int found_vm_pgoff = 0;

				printk_debug(KERN_EMERG "init_vm_file_offset addr_val1 == addr_val2 %zd:%zd:%p:%zu\n", g_vm_file_offset, accurate_offset, rp, addr_val1);
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
					rp += (size_t)sizeof(unsigned long);
					rp += (size_t)sizeof(struct file *);

					addr_val1 = *(size_t*)(rp);
					rp += (size_t)sizeof(void*);
					addr_val2 = *(size_t*)(rp);

					if (addr_val1 == 0 && addr_val2 == 0) {
						g_vm_file_offset += sizeof(void*) * 2;
						g_vm_file_offset += vm_pgoff_offset;
						g_vm_file_offset += sizeof(unsigned long);
						printk_debug(KERN_EMERG "init_vm_file_offset ok, addr_val1 == addr_val2 == 0 %zd:%d\n", g_vm_file_offset, vm_pgoff_offset);
						is_found_vm_file_offset = 1;
						break;
					}

				}


			}
		}
	}

	up_read_mmap_lock(mm);
	mmput(mm);

	if (!is_found_vm_file_offset) {	
		printk_debug(KERN_INFO "find vm_file offset failed\n");
		return -ESPIPE;
	}
	g_init_vm_file_offset_success = true;
	return 0;
}
#endif

static inline struct file * get_vm_file(struct vm_area_struct *vma) {
	struct file * vm_file;
	ssize_t accurate_offset;
	if (g_init_vm_file_offset_success == false) {
		if (init_vm_file_offset() != 0) {
			return NULL;
		}
	}

	accurate_offset = (ssize_t)((size_t)&vma->vm_file - (size_t)vma + g_vm_file_offset);
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