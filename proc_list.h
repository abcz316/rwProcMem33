#ifndef PROC_LIST_H_
#define PROC_LIST_H_
#include "api_proxy.h"
#include "ver_control.h"

#ifndef for_each_process
#define for_each_process(p) \
	for (p = &init_task ; (p = next_task(p)) != &init_task ; )
#endif

#ifndef next_task
#define next_task(p) \
	list_entry_rcu((p)->tasks.next, struct task_struct, tasks)
#endif


//声明
//////////////////////////////////////////////////////////////////////////
MY_STATIC ssize_t get_proc_pid_list(bool is_lookup_proc_file_mode, char* lpBuf, size_t buf_size, bool is_kernel_buf);


//实现
//////////////////////////////////////////////////////////////////////////
MY_STATIC ssize_t get_proc_pid_list(bool is_lookup_proc_file_mode/*闲置状态*/, char* lpBuf, size_t buf_size, bool is_kernel_buf) {
	
	return 0;

	//TODO：这种写法有BUG，不同内核的偏移无法自动修正，有空再修吧，反正意义不太大
	//char* buf_proc_pid_list = lpBuf;
	//size_t buf_size_proc_pid_list = buf_size;
	//size_t buf_pos_proc_pid_list = 0;
	//bool is_kernel_buf_pro_pid_list = is_kernel_buf;
	//ssize_t count_pro_pid_list = 0;
	//struct task_struct *p; //pointer to task_struct

	//for_each_process(p) {
	//	int pid = p->pid;
	//	printk_debug(KERN_EMERG "for_each_process:%d", pid);

	//	count_pro_pid_list++;

	//	if (buf_pos_proc_pid_list >= buf_size_proc_pid_list) {
	//		continue;
	//	}

	//	if (is_kernel_buf_pro_pid_list) {
	//		memcpy((void*)((size_t)buf_proc_pid_list + (size_t)buf_pos_proc_pid_list), &pid, sizeof(pid));
	//	} else {
	//		if (!!x_copy_to_user((void*)((size_t)buf_proc_pid_list + (size_t)buf_pos_proc_pid_list), &pid, sizeof(pid))) {
	//			//用户的缓冲区已满，无法再拷贝
	//			buf_size_proc_pid_list = buf_pos_proc_pid_list;
	//		}
	//	}
	//	buf_pos_proc_pid_list += sizeof(pid);

	//}
	//return count_pro_pid_list;
}


#endif /* PROC_LIST_H_ */


