#ifndef _PROC_PID_H_
#define _PROC_PID_H_
#include <linux/ksm.h>
#include <linux/pid.h>
#include "ver_control.h"
static inline struct pid * get_proc_pid_struct(int nPid) {
	return find_get_pid(nPid);
}
static inline int get_proc_pid(struct pid* proc_pid_struct) {
	return proc_pid_struct->numbers[0].nr;
}
static inline void release_proc_pid_struct(struct pid* proc_pid_struct) {
	put_pid(proc_pid_struct);
}

#endif /* _PROC_PID_H_ */
