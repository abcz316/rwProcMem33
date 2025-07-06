#ifndef PROC_ROOT_H_
#define PROC_ROOT_H_
#include <linux/ctype.h>
#include <linux/cred.h>
#include "proc_root_auto_offset.h"
#include "ver_control.h"
//声明
//////////////////////////////////////////////////////////////////////////
static inline int set_process_root(struct pid* proc_pid_struct);


//实现
//////////////////////////////////////////////////////////////////////////
static uint64_t get_cap_ability_max(void) {

#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(5,8,0)
	uint64_t cap_default = 0x3FFFFFFFFF;
#elif MY_LINUX_VERSION_CODE < KERNEL_VERSION(5,9,0)
	uint64_t cap_default = 0xFFFFFFFFFF;
#else
	uint64_t cap_default = 0x1FFFFFFFFFF;
#endif

	return cap_default;
}

static inline int set_process_root(struct pid* proc_pid_struct) {
	if (g_init_real_cred_offset_success == false) {
		return -ENOENT;
	}

	if (g_init_real_cred_offset_success) {
		struct task_struct * task = NULL;
		struct cred * cred = NULL;
		char *pCred = NULL;
		task = pid_task(proc_pid_struct, PIDTYPE_PID);
		if (!task) { return -1; }

		pCred = (char*)&task->real_cred;

		pCred += g_real_cred_offset;
		pCred += sizeof(void*);
		cred = (struct cred *)*(size_t*)pCred;
		if (cred) {
			uint64_t cap = get_cap_ability_max();
			cred->uid = cred->suid = cred->euid = cred->fsuid = GLOBAL_ROOT_UID;
			cred->gid = cred->sgid = cred->egid = cred->fsgid = GLOBAL_ROOT_GID;
			memcpy(&cred->cap_inheritable, &cap, sizeof(cap));
			memcpy(&cred->cap_permitted, &cap, sizeof(cap));
			memcpy(&cred->cap_effective, &cap, sizeof(cap));
			memcpy(&cred->cap_bset, &cap, sizeof(cap));
			memcpy(&cred->cap_ambient, &cap, sizeof(cap));
			return 0;
		}
		return -EBADF;

	}
	return -ESPIPE;

}
#endif /* PROC_ROOT_H_ */


