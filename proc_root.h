#ifndef PROC_ROOT_H_
#define PROC_ROOT_H_
#include <linux/ctype.h>
#include "ver_control.h"
//声明
//////////////////////////////////////////////////////////////////////////
static inline int get_proc_group(struct pid* proc_pid_struct,
	size_t *npOutUID,
	size_t *npOutSUID,
	size_t *npOutEUID,
	size_t *npOutFSUID,
	size_t *npOutGID,
	size_t *npOutSGID,
	size_t *npOutEGID,
	size_t *npOutFSGID);
static inline int set_proc_root(struct pid* proc_pid_struct);


//实现
//////////////////////////////////////////////////////////////////////////
static ssize_t g_real_cred_offset_proc_root = 0; //task_struct里real_cred的偏移位置
static bool g_init_real_cred_offset_success = false; //是否初始化找到过real_cred的偏移位置

static inline int init_proc_root_offset(void)
{
	size_t size = 512;
	char *lpszStatusBuf[2] = { 0 };

	char comm[TASK_COMM_LEN] = { 0 }; //executable name excluding path
	char * lpFind = NULL;

	//有些内核没有导出get_task_comm，所以不能使用
	//get_task_comm(comm, task);
	

	struct file *fp_status = filp_open("/proc/self/status", O_RDONLY, 0);
	mm_segment_t pold_fs;
	if (IS_ERR(fp_status))
	{
		return -ESRCH;
	}
	pold_fs = get_fs();
	set_fs(KERNEL_DS);


	lpszStatusBuf[0] = (char*)kmalloc(size, GFP_KERNEL);
	lpszStatusBuf[1] = (char*)kmalloc(size, GFP_KERNEL);
	memset(lpszStatusBuf[0], 0, size);
	memset(lpszStatusBuf[1], 0, size);
	if (fp_status->f_op->read(fp_status, lpszStatusBuf[0], size, &fp_status->f_pos) <= 0)
	{
		printk_debug(KERN_INFO "Failed to do read!");

		set_fs(pold_fs);
		filp_close(fp_status, NULL);
		kfree(lpszStatusBuf[0]);
		kfree(lpszStatusBuf[1]);

		return -ESPIPE;
	}
	set_fs(pold_fs);
	filp_close(fp_status, NULL);

	printk_debug(KERN_INFO "lpszStatusBuf1:%s\n", lpszStatusBuf[0]);

	lpFind = strchr(lpszStatusBuf[0], '\n');
	if (lpFind)
	{
		*lpFind = '\0';
	}

	printk_debug(KERN_INFO "lpszStatusBuf2:%s\n", lpszStatusBuf[0]);

	sscanf(lpszStatusBuf[0], "Name: %s", lpszStatusBuf[1]);
	strlcpy(comm, lpszStatusBuf[1], sizeof(comm));
	
	printk_debug(KERN_EMERG "len:%d, comm:%s\n", strlen(comm), comm);


	//
	//for (i = 0; i < TASK_COMM_LEN; i++)
	//{
	//	printk_debug(KERN_EMERG "comm %x\n", comm[i]);
	//}
	kfree(lpszStatusBuf[0]);
	kfree(lpszStatusBuf[1]);



	g_init_real_cred_offset_success = false;



	for (g_real_cred_offset_proc_root = -100; g_real_cred_offset_proc_root <= 300; g_real_cred_offset_proc_root++)
	{
		char *lpszComm = (char*)&current->real_cred;
		lpszComm += g_real_cred_offset_proc_root;

		printk_debug(KERN_EMERG " %x\n", *(unsigned char*)lpszComm);

		if (strcmp(lpszComm, comm) == 0)
		{
			g_real_cred_offset_proc_root -= sizeof(void*) * 2;

			printk_debug(KERN_EMERG "strcmp %zd\n", g_real_cred_offset_proc_root);

			g_init_real_cred_offset_success = true;
			break;
		}

	}


	if (!g_init_real_cred_offset_success)
	{
		printk_debug(KERN_INFO "real_cred offset failed\n");
		return -ESPIPE;
	}
	printk_debug(KERN_INFO "g_real_cred_offset_proc_root:%zu\n", g_real_cred_offset_proc_root);
	return 0;
}
static inline int get_proc_group(struct pid* proc_pid_struct,
	size_t *npOutUID,
	size_t *npOutSUID,
	size_t *npOutEUID,
	size_t *npOutFSUID,
	size_t *npOutGID,
	size_t *npOutSGID,
	size_t *npOutEGID,
	size_t *npOutFSGID)
{
	if (g_init_real_cred_offset_success == false)
	{
		int ret = 0;
		if ((ret = init_proc_root_offset()) != 0)
		{
			return ret;
		}
	}

	if (g_init_real_cred_offset_success)
	{
		struct task_struct * task = NULL;
		struct cred * real_cred = NULL;
		struct cred * cred = NULL;
		char *pCred = NULL;
	
		task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
		if (!task) { return -1; }

		pCred = (char*)&task->real_cred;
		put_task_struct(task);

		pCred += g_real_cred_offset_proc_root;
		real_cred = (struct cred *)*(size_t*)pCred;


		pCred += sizeof(void*);
		cred = (struct cred *)*(size_t*)pCred;

		

		if (!real_cred && !cred) { return -2; }

		
		memset(npOutUID, 0xFF, sizeof(*npOutUID));
		memset(npOutSUID, 0xFF, sizeof(*npOutSUID));
		memset(npOutEUID, 0xFF, sizeof(*npOutEUID));
		memset(npOutFSUID, 0xFF, sizeof(*npOutFSUID));
		memset(npOutGID, 0xFF, sizeof(*npOutGID));
		memset(npOutSGID, 0xFF, sizeof(*npOutSGID));
		memset(npOutEGID, 0xFF, sizeof(*npOutEGID));
		memset(npOutFSGID, 0xFF, sizeof(*npOutFSGID));

		if (real_cred)
		{
			*npOutUID = (size_t)real_cred->uid.val;
			*npOutSUID = (size_t)real_cred->suid.val;
			*npOutEUID = (size_t)real_cred->euid.val;
			*npOutFSUID = (size_t)real_cred->fsuid.val;

			*npOutGID = (size_t)real_cred->gid.val;
			*npOutSGID = (size_t)real_cred->sgid.val;
			*npOutEGID = (size_t)real_cred->egid.val;
			*npOutFSGID = (size_t)real_cred->fsgid.val;

		}
		if (cred)
		{
			*npOutUID = (size_t)cred->uid.val < *npOutUID ? (size_t)cred->uid.val : *npOutUID;
			*npOutSUID = (size_t)cred->suid.val < *npOutSUID ? (size_t)cred->suid.val : *npOutSUID;
			*npOutEUID = (size_t)cred->euid.val < *npOutEUID ? (size_t)cred->euid.val : *npOutEUID;
			*npOutFSUID = (size_t)cred->fsuid.val < *npOutFSUID ? (size_t)cred->fsuid.val : *npOutFSUID;
			*npOutGID = (size_t)cred->gid.val < *npOutGID ? (size_t)cred->gid.val : *npOutGID;
			*npOutSGID = (size_t)cred->sgid.val < *npOutSGID ? (size_t)cred->sgid.val : *npOutSGID;
			*npOutEGID = (size_t)cred->egid.val < *npOutEGID ? (size_t)cred->egid.val : *npOutEGID;
			*npOutFSGID = (size_t)cred->fsgid.val < *npOutFSGID ? (size_t)cred->fsgid.val : *npOutFSGID;

		}
		return 0;

	}
	return -ESPIPE;

}
static inline int set_proc_root(struct pid* proc_pid_struct)
{
	if (g_init_real_cred_offset_success == false)
	{
		int ret = 0;
		if ((ret = init_proc_root_offset()) != 0)
		{
			return ret;
		}
	}

	if (g_init_real_cred_offset_success)
	{
		struct task_struct * task = NULL;
		struct cred * real_cred = NULL;
		struct cred * cred = NULL;
		char *pCred = NULL;
		task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
		if (!task) { return -1; }

		pCred = (char*)&task->real_cred;
		put_task_struct(task);

		pCred += g_real_cred_offset_proc_root;
		real_cred = (struct cred *)*(size_t*)pCred;


		pCred += sizeof(void*);
		cred = (struct cred *)*(size_t*)pCred;

		

		if (!real_cred && !cred) { return -2; }

		if (real_cred)
		{
			real_cred->uid = real_cred->suid = real_cred->euid = real_cred->fsuid = GLOBAL_ROOT_UID;
			real_cred->gid = real_cred->sgid = real_cred->egid = real_cred->fsgid = GLOBAL_ROOT_GID;

		}
		if (cred)
		{
			cred->uid = cred->suid = cred->euid = cred->fsuid = GLOBAL_ROOT_UID;
			cred->gid = cred->sgid = cred->egid = cred->fsgid = GLOBAL_ROOT_GID;
		}
		return 0;

	}
	return -ESPIPE;

}
#endif /* PROC_ROOT_H_ */


