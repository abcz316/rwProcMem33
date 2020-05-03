#ifndef REMOTE_PROC_CMDLINE_H_
#define REMOTE_PROC_CMDLINE_H_
#include <linux/pid.h>
struct pid * get_proc_pid_struct(int nPid);
int get_proc_pid(struct pid* proc_pid_struct);
void release_proc_pid_struct(struct pid* proc_pid_struct);

int get_proc_cmdline_addr(struct pid* proc_pid_struct, ssize_t relative_offset, size_t * arg_start, size_t * arg_end);
#endif /* REMOTE_PROC_CMDLINE_H_ */