#ifndef PROC_LIST_H_
#define PROC_LIST_H_
#include "api_proxy.h"
#include "proc_list_auto_offset.h"
#include "ver_control.h"

#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/errno.h>

//声明
//////////////////////////////////////////////////////////////////////////
static ssize_t get_proc_pid_list(bool is_kernel_buf, char* buf, size_t buf_size);


//实现
//////////////////////////////////////////////////////////////////////////

static ssize_t get_proc_pid_list(bool is_kernel_buf,
                                    char* buf,
                                    size_t buf_size) {
    struct task_struct *p, *next;
    ssize_t count = 0;
    size_t buf_pos = 0;

    if (!g_init_task_next_offset_success || !g_init_task_pid_offset_success) {
        return -EFAULT;
    }

    p = &init_task;
    while (1) {
        uintptr_t list_next = *(uintptr_t *)((char *)p + g_task_next_offset);
        next = (struct task_struct *)(list_next - g_task_next_offset);
        if (next == &init_task)
            break;
        
        count++;

        // copy pid
        {
            pid_t pid_v = *(pid_t *)((char *)next + g_task_pid_offset);
            int pid_n = pid_v;
            printk_debug(KERN_INFO "iter_task: pid = %d\n", pid_n);
            if (buf_pos < buf_size) {
                if (is_kernel_buf) {
                    memcpy((void*)((size_t)buf + (size_t)buf_pos), &pid_n, sizeof(pid_n));
                } else {
                    x_copy_to_user((void*)((size_t)buf + (size_t)buf_pos), &pid_n, sizeof(pid_n));
                }
                buf_pos += sizeof(pid_n);
            }
        }
        p = next;
    }

    return count;
}

#endif /* PROC_LIST_H_ */


