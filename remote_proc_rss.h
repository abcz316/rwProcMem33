#ifndef REMOTE_PROC_RSS_H_
#define REMOTE_PROC_RSS_H_
#include <linux/pid.h>
size_t read_proc_rss_size(struct pid* proc_pid_struct, ssize_t relative_offset);
#endif /* REMOTE_PROC_RSS_H_ */