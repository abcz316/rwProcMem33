#ifndef _HWBP_PROC_H_
#define _HWBP_PROC_H_
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/compat.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/kallsyms.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <linux/ksm.h>
#include <linux/mutex.h>
#include <linux/ktime.h>
#include <linux/pid.h>
#include <linux/slab.h> //kmalloc与kfree
#include "ver_control.h"
#include "arm64_register_helper.h"
#include "cvector.h"
#ifdef CONFIG_USE_PROC_FILE_NODE
#include <linux/proc_fs.h>
#include "hide_procfs_dir.h"
#endif
//////////////////////////////////////////////////////////////////

enum {
	CMD_OPEN_PROCESS, 				// 打开进程
	CMD_CLOSE_PROCESS, 				// 关闭进程
	CMD_GET_NUM_BRPS, 				// 获取CPU硬件执行断点支持数量
	CMD_GET_NUM_WRPS, 				// 获取CPU硬件访问断点支持数量
	CMD_INST_PROCESS_HWBP,			// 安装进程硬件断点
	CMD_UNINST_PROCESS_HWBP,		// 卸载进程硬件断点
	CMD_SUSPEND_PROCESS_HWBP,		// 暂停进程硬件断点
	CMD_RESUME_PROCESS_HWBP,		// 恢复进程硬件断点
	CMD_GET_HWBP_HIT_COUNT,			// 获取硬件断点命中地址数量
	CMD_GET_HWBP_HIT_DETAIL,		// 获取硬件断点命中详细信息
	CMD_SET_HOOK_PC,				// 设置无条件Hook跳转
	CMD_HIDE_KERNEL_MODULE,			// 隐藏驱动
};

struct hwBreakpointProcDev {
#ifdef CONFIG_USE_PROC_FILE_NODE
	struct proc_dir_entry *proc_parent;
	struct proc_dir_entry *proc_entry;
#endif
	bool is_hidden_module; //是否已经隐藏过驱动列表了
};
static struct hwBreakpointProcDev *g_hwBreakpointProc_devp;

static ssize_t hwBreakpointProc_read(struct file* filp, char __user* buf, size_t size, loff_t* ppos);
static int hwBreakpointProc_release(struct inode *inode, struct file *filp);
static const struct proc_ops hwBreakpointProc_proc_ops = {
    .proc_read    = hwBreakpointProc_read,
	.proc_release = hwBreakpointProc_release,
};

#pragma pack(1)
struct my_user_pt_regs {
	uint64_t regs[31];
	uint64_t sp;
	uint64_t pc;
	uint64_t pstate;
	uint64_t orig_x0;
	uint64_t syscallno;
};
struct HWBP_HIT_ITEM {
	uint64_t task_id;
	uint64_t hit_addr;
	uint64_t hit_time;
	struct my_user_pt_regs regs_info;
};
#pragma pack()

struct HWBP_HANDLE_INFO {
	uint64_t task_id;
	struct perf_event * sample_hbp;
	struct perf_event_attr original_attr;
	bool is_32bit_task;
#ifdef CONFIG_MODIFY_HIT_NEXT_MODE
	struct perf_event_attr next_instruction_attr;
#endif
	size_t hit_total_count;
	cvector hit_item_arr;
};

#endif /* _HWBP_PROC_H_ */