#ifndef PROC_LIST_AUTO_OFFSET_H_
#define PROC_LIST_AUTO_OFFSET_H_
#include <linux/ctype.h>
#include "ver_control.h"

static ssize_t g_task_next_offset = 0;
static bool g_init_task_next_offset_success = false;

static ssize_t g_task_pid_offset = 0;
static bool g_init_task_pid_offset_success = false;

static inline int init_task_next_offset(void) {
    struct task_struct *mytask = x_get_current();
    struct mm_struct  *mm    = get_task_mm(mytask);
    size_t              off_mm = 0;
    size_t              off    = 0;
    uintptr_t           addr_mytask;
    uintptr_t           addr_mm;
    if (!mm)
        return -ESRCH;
    if (g_init_task_next_offset_success) {
        mmput(mm);
        return 0;
    }

    addr_mytask = (uintptr_t)mytask;
    addr_mm     = (uintptr_t)mm;

    /* 寻找 mm_struct 在 task_struct 中的偏移 */
    for (off = 0; off <= sizeof(*mytask) - sizeof(void*); off += 4) {
        void *v = *(void **)(addr_mytask + off);
        if ((uintptr_t)v == addr_mm) {
            off_mm = off;
            break;
        }
    }

    if (off_mm == 0) {
        printk_debug(KERN_EMERG "init_task_next_offset mm_struct failed.\n");
        mmput(mm);
        return -EFAULT;
    }

    g_task_next_offset = off_mm
                         - sizeof(mytask->pushable_dl_tasks)
                         - sizeof(mytask->pushable_tasks)
                         - sizeof(mytask->tasks);
    g_init_task_next_offset_success = true;
    printk_debug(KERN_INFO "init_task_next_offset: found tasks offset = %zu bytes\n",
                 g_task_next_offset);

    mmput(mm);
    return 0;
}


static inline int init_task_pid_offset(int pid, int tgid) {
    struct task_struct *mytask = x_get_current();
    struct mm_struct   *mm     = get_task_mm(mytask);
    uintptr_t           addr_mytask = (uintptr_t)mytask;
    uintptr_t           addr_mm     = (uintptr_t)mm;
    size_t              off_mm    = 0;
    size_t              off       = 0;
    size_t off_pid_static = (uintptr_t)&mytask->pid - addr_mytask;

    printk_debug(KERN_INFO
        "init_task_pid_offset: mytask@%p, &pid@%p, static off_pid = %zu\n",
        mytask, &mytask->pid, off_pid_static);

    if (!mm)
        return -ESRCH;

    if (g_init_task_pid_offset_success) {
        mmput(mm);
        return 0;
    }

    for (off = 0; off <= sizeof(*mytask) - sizeof(void*); off += 4) {
        void *v = *(void **)(addr_mytask + off);
        if ((uintptr_t)v == addr_mm) {
            off_mm = off;
            break;
        }
    }
    if (off_mm == 0) {
        printk_debug(KERN_EMERG "init_task_pid_offset: mm_struct offset not found\n");
        mmput(mm);
        return -EFAULT;
    }
    printk_debug(KERN_INFO "init_task_pid_offset: mm_struct offset found = %zu\n", off_mm);

    /* 2) 从 mm_struct 偏移处开始，搜索 pid 和 tgid */
    for (off = off_mm; off <= sizeof(*mytask) - 2 * sizeof(pid_t); off += 4) {
        pid_t pid_v  = *(pid_t *)(addr_mytask + off);
        pid_t tgid_v = *(pid_t *)(addr_mytask + off + sizeof(pid_t));
        if (pid_v == pid && tgid_v == tgid) {
            g_task_pid_offset = off;
            g_init_task_pid_offset_success = true;
            printk_debug(KERN_INFO
                "init_task_pid_offset: found pid/tgid offset = %zu (pid=%d, tgid=%d)\n",
                g_task_pid_offset, pid, tgid);
            mmput(mm);
            return 0;
        }
    }

    printk_debug(KERN_EMERG
        "init_task_pid_offset: failed to match pid=%d, tgid=%d\n",
        pid, tgid);
    mmput(mm);
    return -ENOENT;
}


#endif /* PROC_LIST_AUTO_OFFSET_H_ */


