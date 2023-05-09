#ifndef PROC_ROOT_H_
#define PROC_ROOT_H_
#include <linux/ctype.h>
#include <linux/cred.h>
#include "proc_root_auto_offset.h"
#include "ver_control.h"
//声明
//////////////////////////////////////////////////////////////////////////
MY_STATIC inline int get_proc_group(struct pid* proc_pid_struct,
	size_t *npOutUID,
	size_t *npOutSUID,
	size_t *npOutEUID,
	size_t *npOutFSUID,
	size_t *npOutGID,
	size_t *npOutSGID,
	size_t *npOutEGID,
	size_t *npOutFSGID);
MY_STATIC inline int set_proc_root(struct pid* proc_pid_struct);


//实现
//////////////////////////////////////////////////////////////////////////
MY_STATIC inline int get_proc_group(struct pid* proc_pid_struct,
	size_t *npOutUID,
	size_t *npOutSUID,
	size_t *npOutEUID,
	size_t *npOutFSUID,
	size_t *npOutGID,
	size_t *npOutSGID,
	size_t *npOutEGID,
	size_t *npOutFSGID) {
	if (g_init_real_cred_offset_success == false) {
		return -ENOENT;
	}

	if (g_init_real_cred_offset_success) {
		struct task_struct * task = NULL;
		struct cred * real_cred = NULL;
		struct cred * cred = NULL;
		char *pCred = NULL;

		task = pid_task(proc_pid_struct, PIDTYPE_PID);
		if (!task) { return -1; }

		pCred = (char*)&task->real_cred;

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

		if (real_cred) {
			unsigned int tmp = 0;
			memcpy(&tmp, &real_cred->uid, sizeof(tmp));
			*npOutUID = (size_t)tmp;

			memcpy(&tmp, &real_cred->suid, sizeof(tmp));
			*npOutSUID = (size_t)tmp;

			memcpy(&tmp, &real_cred->euid, sizeof(tmp));
			*npOutEUID = (size_t)tmp;

			memcpy(&tmp, &real_cred->fsuid, sizeof(tmp));
			*npOutFSUID = (size_t)tmp;

			memcpy(&tmp, &real_cred->gid, sizeof(tmp));
			*npOutGID = (size_t)tmp;

			memcpy(&tmp, &real_cred->sgid, sizeof(tmp));
			*npOutSGID = (size_t)tmp;

			memcpy(&tmp, &real_cred->egid, sizeof(tmp));
			*npOutEGID = (size_t)tmp;

			memcpy(&tmp, &real_cred->fsgid, sizeof(tmp));
			*npOutFSGID = (size_t)tmp;

		}
		return 0;

	}
	return -ESPIPE;

}
MY_STATIC inline int set_proc_root(struct pid* proc_pid_struct) {
	if (g_init_real_cred_offset_success == false) {
		return -ENOENT;
	}

	if (g_init_real_cred_offset_success) {
		struct task_struct * task = NULL;
		struct cred * real_cred = NULL;
		struct cred * cred = NULL;
		char *pCred = NULL;
		task = pid_task(proc_pid_struct, PIDTYPE_PID);
		if (!task) { return -1; }

		pCred = (char*)&task->real_cred;

		pCred += g_real_cred_offset_proc_root;
		real_cred = (struct cred *)*(size_t*)pCred;


		pCred += sizeof(void*);
		cred = (struct cred *)*(size_t*)pCred;



		if (!real_cred && !cred) { return -2; }

		if (real_cred) {
			real_cred->uid = real_cred->suid = real_cred->euid = real_cred->fsuid = GLOBAL_ROOT_UID;
			real_cred->gid = real_cred->sgid = real_cred->egid = real_cred->fsgid = GLOBAL_ROOT_GID;

			memset(&real_cred->cap_inheritable, 0xFF, sizeof(real_cred->cap_inheritable));
			memset(&real_cred->cap_permitted, 0xFF, sizeof(real_cred->cap_permitted));
			memset(&real_cred->cap_effective, 0xFF, sizeof(real_cred->cap_effective));
			memset(&real_cred->cap_bset, 0xFF, sizeof(real_cred->cap_bset));
			memset(&real_cred->cap_ambient, 0xFF, sizeof(real_cred->cap_ambient));
			

		}
		if (cred) {
			cred->uid = cred->suid = cred->euid = cred->fsuid = GLOBAL_ROOT_UID;
			cred->gid = cred->sgid = cred->egid = cred->fsgid = GLOBAL_ROOT_GID;
			memset(&cred->cap_inheritable, 0xFF, sizeof(cred->cap_inheritable));
			memset(&cred->cap_permitted, 0xFF, sizeof(cred->cap_permitted));
			memset(&cred->cap_effective, 0xFF, sizeof(cred->cap_effective));
			memset(&cred->cap_bset, 0xFF, sizeof(cred->cap_bset));
			memset(&cred->cap_ambient, 0xFF, sizeof(cred->cap_ambient));

		}

		return 0;

	}
	return -ESPIPE;

}
#endif /* PROC_ROOT_H_ */


