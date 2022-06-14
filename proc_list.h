#ifndef PROC_LIST_H_
#define PROC_LIST_H_
#include <linux/ctype.h>
#include "ver_control.h"
//声明
//////////////////////////////////////////////////////////////////////////
MY_STATIC ssize_t get_proc_pid_list(bool is_lookup_proc_file_mode, char* lpBuf, size_t buf_size, bool is_kernel_buf);


//实现
//////////////////////////////////////////////////////////////////////////
MY_STATIC struct semaphore g_sema_proc_pid_list;
MY_STATIC char* g_buf_proc_pid_list = NULL;
MY_STATIC size_t g_buf_size_proc_pid_list;
MY_STATIC size_t g_buf_pos_proc_pid_list;
MY_STATIC bool g_is_kernel_buf_pro_pid_list;
MY_STATIC ssize_t g_count_pro_pid_list;


MY_STATIC inline int atoi(const char arr[]) {
	int index = 0;
	int flag = 1;
	int num = 0;

	if (arr == NULL) { return -1; }
	while (isspace(arr[index])) { index++; }
	if (arr[index] == '-') { flag = -1; }
	if (arr[index] == '-' || arr[index] == '+') { index++; }
	while (arr[index] >= '0' && arr[index] <= '9') { num = num * 10 + arr[index] - '0';	index++; }
	return flag * num;
}

MY_STATIC int handle_proc_pid_filldir(struct dir_context *ctx, const char *d_name, int namlen, loff_t offset, u64 ino, unsigned d_type) {


	printk_debug(KERN_EMERG "handle_proc_pid_filldir d_name: %s", d_name);



	if (strspn(d_name, "1234567890") == strlen(d_name) && d_type == DT_DIR) {
		int pid;

		g_count_pro_pid_list++;

		if (g_buf_pos_proc_pid_list >= g_buf_size_proc_pid_list) {
			return 0;
		}

		pid = atoi(d_name);
		if (g_is_kernel_buf_pro_pid_list) {
			memcpy((void*)((size_t)g_buf_proc_pid_list + (size_t)g_buf_pos_proc_pid_list), &pid, sizeof(pid));
		} else {
			if (!!copy_to_user((void*)((size_t)g_buf_proc_pid_list + (size_t)g_buf_pos_proc_pid_list), &pid, sizeof(pid))) {
				//�û��Ļ������������޷��ٿ���
				g_buf_size_proc_pid_list = g_buf_pos_proc_pid_list;
			}
		}
		g_buf_pos_proc_pid_list += sizeof(pid);
	}
	return 0;
}


MY_STATIC ssize_t get_proc_pid_list(bool is_lookup_proc_file_mode, char* lpBuf, size_t buf_size, bool is_kernel_buf) {

	if (is_lookup_proc_file_mode) {
		struct dir_context dctx;
		struct file *filp = NULL;
		void *pfunc = handle_proc_pid_filldir;

		if (g_buf_proc_pid_list == NULL) {
			//��ʼ���ź���
			sema_init(&g_sema_proc_pid_list, 1);
			g_buf_proc_pid_list = NULL;
			g_buf_size_proc_pid_list = 0;
			g_buf_pos_proc_pid_list = 0;
			g_is_kernel_buf_pro_pid_list = true;
			g_count_pro_pid_list = 0;
		}

		memcpy((void*)&dctx.actor, &pfunc, sizeof(pfunc)); //dctx->actor = fake_filldir;
		dctx.pos = 0;
		filp = filp_open("/proc", O_RDONLY, 0);
		if (IS_ERR(filp)) {
			return -1;
		}

		down(&g_sema_proc_pid_list);
		g_buf_proc_pid_list = lpBuf;
		g_buf_size_proc_pid_list = buf_size;
		g_buf_pos_proc_pid_list = 0;
		g_is_kernel_buf_pro_pid_list = is_kernel_buf;
		g_count_pro_pid_list = 0;

		filp->f_op->FILE_OP_DIR_ITER(filp, &dctx);

		up(&g_sema_proc_pid_list);

		//S_IFSOCK
		filp_close(filp, NULL);
		return g_count_pro_pid_list;
	} else {
		char* buf_proc_pid_list = lpBuf;
		size_t buf_size_proc_pid_list = buf_size;
		size_t buf_pos_proc_pid_list = 0;
		bool is_kernel_buf_pro_pid_list = is_kernel_buf;
		ssize_t count_pro_pid_list = 0;
		struct task_struct *p; //pointer to task_struct

		for_each_process(p) {
			int pid = p->pid;
			printk_debug(KERN_EMERG "for_each_process:%d", pid);

			count_pro_pid_list++;

			if (buf_pos_proc_pid_list >= buf_size_proc_pid_list) {
				continue;
			}

			if (is_kernel_buf_pro_pid_list) {
				memcpy((void*)((size_t)buf_proc_pid_list + (size_t)buf_pos_proc_pid_list), &pid, sizeof(pid));
			} else {
				if (!!copy_to_user((void*)((size_t)buf_proc_pid_list + (size_t)buf_pos_proc_pid_list), &pid, sizeof(pid))) {
					//用户的缓冲区已满，无法再拷贝
					buf_size_proc_pid_list = buf_pos_proc_pid_list;
				}
			}
			buf_pos_proc_pid_list += sizeof(pid);

		}
		return count_pro_pid_list;
	}


}


#endif /* PROC_LIST_H_ */


