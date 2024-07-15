#ifndef PROC_ROOT_AUTO_OFFSET_H_
#define PROC_ROOT_AUTO_OFFSET_H_
#include <linux/ctype.h>
#include "ver_control.h"

MY_STATIC ssize_t g_real_cred_offset_proc_root = 0;
MY_STATIC bool g_init_real_cred_offset_success = false;

MY_STATIC inline int init_proc_root_offset(const char* proc_self_status_content) {
	size_t len = 0;
	char* lp_tmp_line = NULL;
	char* lp_tmp_name = NULL;

	char comm[TASK_COMM_LEN] = { 0 }; //executable name excluding path
	char* lp_find_line_single = NULL;
	const ssize_t offset_lookup_min = -100;
	const ssize_t offset_lookup_max = 300;
	const ssize_t min_real_cred_offset_safe = offset_lookup_min + sizeof(void*) * 3;

	//有些内核没有导出get_task_comm，所以不能使用
	//get_task_comm(comm, task);

	lp_find_line_single = strchr(proc_self_status_content, '\n');
	if (!lp_find_line_single) {
		return 0;
	}

	len = ((size_t)lp_find_line_single - (size_t)proc_self_status_content);
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

	printk_debug(KERN_EMERG "len:%lu, comm:%s\n", strlen(comm), comm);


	//
	//for (i = 0; i < TASK_COMM_LEN; i++)
	//{
	//	printk_debug(KERN_EMERG "comm %x\n", comm[i]);
	//}


	kfree(lp_tmp_line);
	kfree(lp_tmp_name);


	g_init_real_cred_offset_success = false;

	for (g_real_cred_offset_proc_root = offset_lookup_min; g_real_cred_offset_proc_root <= offset_lookup_max; g_real_cred_offset_proc_root++) {

		char* lpszComm = (char*)&current->real_cred;
		lpszComm += g_real_cred_offset_proc_root;

		printk_debug(KERN_EMERG "curent g_real_cred_offset_proc_root:%zd, bytes:%x\n", g_real_cred_offset_proc_root, *(unsigned char*)lpszComm);

		if(g_real_cred_offset_proc_root < min_real_cred_offset_safe) {
			continue;
		}
		if (strcmp(lpszComm, comm) == 0) {
			ssize_t maybe_real_cred_offset = g_real_cred_offset_proc_root - sizeof(void*) * 2;
			char * p_test_mem1 = (char*)&current->real_cred + maybe_real_cred_offset;
			char * p_test_mem2 = (char*)&current->real_cred + maybe_real_cred_offset + sizeof(void*);
			if(memcmp(p_test_mem1, p_test_mem2, sizeof(void*)) != 0 ) {
				maybe_real_cred_offset = g_real_cred_offset_proc_root - sizeof(void*) * 3;
				p_test_mem1 = (char*)&current->real_cred + maybe_real_cred_offset;
				p_test_mem2 = (char*)&current->real_cred + maybe_real_cred_offset + sizeof(void*);
				if(memcmp(p_test_mem1, p_test_mem2, sizeof(void*)) != 0 ) {
					break; // failed
				}
			}

			g_real_cred_offset_proc_root = maybe_real_cred_offset;

			printk_debug(KERN_EMERG "strcmp found %zd\n", g_real_cred_offset_proc_root);

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


