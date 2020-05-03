#ifndef REMOTE_PROC_MAPS_H_
#define REMOTE_PROC_MAPS_H_
#include <linux/pid.h>
#include <linux/types.h>
#include <linux/mm_types.h>

size_t get_proc_map_count(struct pid* proc_pid_struct);
int get_proc_maps_list_to_kernel(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int *have_pass);
int get_proc_maps_list_to_user(struct pid* proc_pid_struct, size_t max_path_length, char* lpBuf, size_t buf_size, int *have_pass);


bool check_proc_map_can_read(struct pid* proc_pid_struct, size_t proc_virt_addr, size_t size);
bool check_proc_map_can_write(struct pid* proc_pid_struct, size_t proc_virt_addr, size_t size);

#endif /* REMOTE_PROC_MAPS_H_ */