#ifndef PROC_ROOT_AUTO_OFFSET_H_
#define PROC_ROOT_AUTO_OFFSET_H_
#include <linux/ctype.h>
#include "ver_control.h"

MY_STATIC ssize_t g_real_cred_offset_proc_root = 0; //task_struct里real_cred的偏移位置
MY_STATIC bool g_init_real_cred_offset_success = false; //是否初始化找到过real_cred的偏移位置

MY_STATIC inline int init_proc_root_offset(const char* proc_self_status_content) {
	size_t len = 0;
	char* lp_tmp_line = NULL;
	char* lp_tmp_name = NULL;

	char comm[TASK_COMM_LEN] = { 0 }; //executable name excluding path
	char* lpFind = NULL;

	//有些内核没有导出get_task_comm，所以不能使用
	//get_task_comm(comm, task);

	lpFind = strchr(proc_self_status_content, '\n');
	if (!lpFind) {
		return 0;
	}

	len = ((size_t)lpFind - (size_t)proc_self_status_content);
	lp_tmp_line = (char*)kmalloc(len + 1, GFP_KERNEL);
	lp_tmp_name = (char*)kmalloc(len + 1, GFP_KERNEL);
	if (!lp_tmp_line || !lp_tmp_name) {
		return -ENOMEM;
	}
	memset(lp_tmp_line, 0, len + 1);
	memset(lp_tmp_name, 0, len + 1);
	memcpy(lp_tmp_line, proc_self_status_content, len);

	printk_debug(KERN_INFO "lp_tmp_line:%s\n", lp_tmp_line);




	sscanf(lp_tmp_line, "Name: %s", lp_tmp_name);
	strlcpy(comm, lp_tmp_name, sizeof(comm));

	printk_debug(KERN_EMERG "len:%d, comm:%s\n", strlen(comm), comm);


	//
	//for (i = 0; i < TASK_COMM_LEN; i++)
	//{
	//	printk_debug(KERN_EMERG "comm %x\n", comm[i]);
	//}


	kfree(lp_tmp_line);
	kfree(lp_tmp_name);


	g_init_real_cred_offset_success = false;



	for (g_real_cred_offset_proc_root = -100; g_real_cred_offset_proc_root <= 300; g_real_cred_offset_proc_root++) {
		char* lpszComm = (char*)&current->real_cred;
		lpszComm += g_real_cred_offset_proc_root;

		printk_debug(KERN_EMERG " %x\n", *(unsigned char*)lpszComm);

		if (strcmp(lpszComm, comm) == 0) {
			g_real_cred_offset_proc_root -= sizeof(void*) * 2;

			printk_debug(KERN_EMERG "strcmp %zd\n", g_real_cred_offset_proc_root);

			g_init_real_cred_offset_success = true;
			break;
		}

	}

	if (!g_init_real_cred_offset_success) {
		printk_debug(KERN_INFO "real_cred offset failed\n");
		return -ESPIPE;
	}
	printk_debug(KERN_INFO "g_real_cred_offset_proc_root:%zu\n", g_real_cred_offset_proc_root);
	return 0;
}
#endif /* PROC_ROOT_AUTO_OFFSET_H_ */


