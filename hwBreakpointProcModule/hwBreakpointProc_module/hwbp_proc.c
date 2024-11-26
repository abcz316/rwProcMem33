
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
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

#include <linux/slab.h>
#include <linux/device.h>
#include "arm64_register_helper.h"
#include "cvector.h"
#include "proc_pid.h"
#include "api_proxy.h"


#define MAJOR_NUM 100
#define IOCTL_HWBP_OPEN_PROCESS 				_IOR(MAJOR_NUM, 1, char*) //打开进程
#define IOCTL_HWBP_CLOSE_HANDLE 				_IOR(MAJOR_NUM, 2, char*) //关闭进程
#define IOCTL_HWBP_GET_NUM_BRPS 				_IOR(MAJOR_NUM, 3, char*) //获取CPU支持硬件执行断点的数量
#define IOCTL_HWBP_GET_NUM_WRPS 				_IOR(MAJOR_NUM, 4, char*) //获取CPU支持硬件访问断点的数量
#define IOCTL_HWBP_INST_PROCESS_HWBP			_IOR(MAJOR_NUM, 5, char*) //设置进程硬件断点
#define IOCTL_HWBP_UNINST_PROCESS_HWBP		_IOR(MAJOR_NUM, 6, char*) //删除进程硬件断点
#define IOCTL_HWBP_SUSPEND_PROCESS_HWBP	_IOR(MAJOR_NUM, 7, char*) //暂停进程硬件断点
#define IOCTL_HWBP_RESUME_PROCESS_HWBP		_IOR(MAJOR_NUM, 8, char*) //恢复进程硬件断点
#define IOCTL_HWBP_GET_HWBP_HIT_COUNT		_IOR(MAJOR_NUM, 9, char*) //获取硬件断点命中地址数量
#define IOCTL_HWBP_SET_HOOK_PC						_IOR(MAJOR_NUM, 20, char*)

static atomic64_t g_hook_pc;


//////////////////////////////////////////////////////////////////
static int g_hwBreakpointProc_major = 0;
static dev_t g_hwBreakpointProc_devno;

struct hwBreakpointProcDev {
	struct cdev *pcdev;
	size_t cur_dev_open_count;
	bool is_already_hide_dev_file;
};
static struct hwBreakpointProcDev *g_hwBreakpointProc_devp;
static struct class *g_Class_devp;

static struct mutex g_hwbp_handle_info_mutex;
static cvector g_hwbp_handle_info_arr = NULL;

static void record_hit_details(struct HWBP_HANDLE_INFO *info, struct pt_regs *regs) {
    struct HWBP_HIT_ITEM hit_item = {0};
	if (!info || !regs) {
        return;
    }
	hit_item.task_id = info->task_id;
    hit_item.hit_addr = regs->pc;
	hit_item.hit_time = ktime_get_real_seconds();
    memcpy(&hit_item.regs_info.regs, regs->regs, sizeof(hit_item.regs_info.regs));
    hit_item.regs_info.sp = regs->sp;
    hit_item.regs_info.pc = regs->pc;
    hit_item.regs_info.pstate = regs->pstate;
    hit_item.regs_info.orig_x0 = regs->orig_x0;
    hit_item.regs_info.syscallno = regs->syscallno;
    if (info->hit_item_arr) {
		if(cvector_length(info->hit_item_arr) < MIN_LEN) { // 最多存放MIN_LEN个
			cvector_pushback(info->hit_item_arr, &hit_item);
		}
    }
}

#ifdef CONFIG_MODIFY_HIT_NEXT_MODE
static bool arm64_move_bp_to_next_instruction(struct perf_event *bp, uint64_t next_instruction_addr, struct perf_event_attr *original_attr, struct perf_event_attr * next_instruction_attr) {
    int result;
	if (!bp || !original_attr || !next_instruction_attr || !next_instruction_addr) {
        return false;
    }
	memcpy(next_instruction_attr, original_attr, sizeof(struct perf_event_attr));
	next_instruction_attr->bp_addr = next_instruction_addr;
	next_instruction_attr->bp_len = HW_BREAKPOINT_LEN_4;
	next_instruction_attr->bp_type = HW_BREAKPOINT_X;
	next_instruction_attr->disabled = 0;
	result = x_modify_user_hw_breakpoint(bp, next_instruction_attr);
	if(result) {
		next_instruction_attr->bp_addr = 0;
		return false;
	}
	return true;
}

static bool arm64_recovery_bp_to_original(struct perf_event *bp, struct perf_event_attr *original_attr, struct perf_event_attr * next_instruction_attr) {
    int result;
	if (!bp || !original_attr || !next_instruction_attr) {
        return false;
    }
	result = x_modify_user_hw_breakpoint(bp, original_attr);
	if(result) {
		return false;
	}
	next_instruction_attr->bp_addr = 0;
	return true;
}
#endif

static void hwbp_hit_user_info_callback(struct perf_event *bp,
	struct perf_sample_data *data,
	struct pt_regs *regs, struct HWBP_HANDLE_INFO * hwbp_handle_info) {
	hwbp_handle_info->hit_total_count++;
	record_hit_details(hwbp_handle_info, regs);
}

/*
 * Handle hitting a HW-breakpoint.
 */
static void hwbp_handler(struct perf_event *bp,
	struct perf_sample_data *data,
	struct pt_regs *regs) {
	citerator iter;
	uint64_t hook_pc;
	printk_debug(KERN_INFO "hw_breakpoint HIT!!!!! bp:%px, pc:%px, id:%d\n", bp, regs->pc, bp->id);

	hook_pc = atomic64_read(&g_hook_pc);
	if(hook_pc) {
		regs->pc = hook_pc;
		return;
	}

	mutex_lock(&g_hwbp_handle_info_mutex);
	for (iter = cvector_begin(g_hwbp_handle_info_arr); iter != cvector_end(g_hwbp_handle_info_arr); iter = cvector_next(g_hwbp_handle_info_arr, iter)) {
		struct HWBP_HANDLE_INFO * hwbp_handle_info = (struct HWBP_HANDLE_INFO *)iter;
		if (hwbp_handle_info->sample_hbp != bp) {
			continue;
		}
#ifdef CONFIG_MODIFY_HIT_NEXT_MODE
		if(hwbp_handle_info->next_instruction_attr.bp_addr != regs->pc) {
			// first hit
			bool should_toggle = true;
			hwbp_hit_user_info_callback(bp, data, regs, hwbp_handle_info);
			if(!hwbp_handle_info->is_32bit_task) {
				if(arm64_move_bp_to_next_instruction(bp, regs->pc + 4, &hwbp_handle_info->original_attr, &hwbp_handle_info->next_instruction_attr)) {
					should_toggle = false;
				}
			}
			if(should_toggle) {
				toggle_bp_registers_directly(&hwbp_handle_info->original_attr, hwbp_handle_info->is_32bit_task, 0);
			}
		} else {
			// second hit
			if(!arm64_recovery_bp_to_original(bp, &hwbp_handle_info->original_attr, &hwbp_handle_info->next_instruction_attr)) {
				toggle_bp_registers_directly(&hwbp_handle_info->next_instruction_attr, hwbp_handle_info->is_32bit_task, 0);
			}
		}
#else
		hwbp_hit_user_info_callback(bp, data, regs, hwbp_handle_info);
		toggle_bp_registers_directly(&hwbp_handle_info->original_attr, hwbp_handle_info->is_32bit_task, 0);
#endif
	}
	
	mutex_unlock(&g_hwbp_handle_info_mutex);
}



static long OnIoctlOpenProcess(unsigned long arg) {
	int64_t pid = 0;
	struct pid * proc_pid_struct = NULL;
	if (copy_from_user(&pid, (void __user *)arg, 8)) {
		return -EINVAL;
	}
	printk_debug(KERN_INFO "pid:%ld\n", pid);
	proc_pid_struct = get_proc_pid_struct(pid);
	printk_debug(KERN_INFO "proc_pid_struct *:%px\n", proc_pid_struct);
	if (!proc_pid_struct) {
		return -EINVAL;
	}
	if (copy_to_user((void*)arg, &proc_pid_struct, 8)) {
		return -EINVAL;
	}
	return 0;
}

static long OnIoctlCloseProcess(unsigned long arg) {
	//获取进程句柄
	struct pid * proc_pid_struct = NULL;
	if (copy_from_user((void*)&proc_pid_struct, (void*)arg, 8)) {
		return -EFAULT;
	}
	//关闭进程句柄
	printk_debug(KERN_INFO "proc_pid_struct *:%px\n", proc_pid_struct);
	release_proc_pid_struct(proc_pid_struct);
	return 0;
}

static long OnIoctlGetCpuNumBrps(void) {
	return getCpuNumBrps();
}

static long OnIoctlGetCpuNumWrps(void) {
	return getCpuNumWrps();
}

static long OnIoctlInstProcessHwbp(unsigned long arg) {
	#pragma pack(1)
	struct buf_layout {
		struct pid * proc_pid_struct;
		size_t proc_virt_addr;
		unsigned int hwbp_len;
		unsigned int hwbp_type;
	};
	#pragma pack()
	pid_t pid_val;
	struct task_struct *task;
	struct buf_layout user_data = {0};
	struct HWBP_HANDLE_INFO hwbp_handle_info = { 0 };
	if (copy_from_user((void*)&user_data, (void*)arg, 24)) {
		return -EFAULT;
	}
	printk_debug(KERN_INFO "proc_pid_struct *:%px\n", user_data.proc_pid_struct);
	printk_debug(KERN_INFO "proc_virt_addr :%px\n", user_data.proc_virt_addr);
	printk_debug(KERN_INFO "hwbp_len:%zu\n", user_data.hwbp_len);
	printk_debug(KERN_INFO "hwbp_type:%d\n", user_data.hwbp_type);
	pid_val = pid_nr(user_data.proc_pid_struct);
	printk_debug(KERN_INFO "pid_val:%d\n", pid_val);

	if (!pid_val) {
		printk_debug(KERN_INFO "pid_nr failed.\n");
		return -EINVAL;
	}

	task = pid_task(user_data.proc_pid_struct, PIDTYPE_PID);
	if (!task) {
		printk_debug(KERN_INFO "get_pid_task failed.\n");
		return -EINVAL;
	}
	
	hwbp_handle_info.task_id = pid_val;
	hwbp_handle_info.is_32bit_task = is_compat_thread(task_thread_info(task));
	ptrace_breakpoint_init(&hwbp_handle_info.original_attr);
	hwbp_handle_info.original_attr.bp_addr = user_data.proc_virt_addr;
	hwbp_handle_info.original_attr.bp_len = user_data.hwbp_len;
	hwbp_handle_info.original_attr.bp_type = user_data.hwbp_type;
	hwbp_handle_info.original_attr.disabled = 0;

	hwbp_handle_info.sample_hbp = x_register_user_hw_breakpoint(&hwbp_handle_info.original_attr, hwbp_handler, NULL, task);
	printk_debug(KERN_INFO "register_user_hw_breakpoint return: %px\n", hwbp_handle_info.sample_hbp);
	if (IS_ERR((void __force *)hwbp_handle_info.sample_hbp)) {
		int ret = PTR_ERR((void __force *)hwbp_handle_info.sample_hbp);
		printk_debug(KERN_INFO "register_user_hw_breakpoint failed: %d\n", ret);
		return ret;
	}
	hwbp_handle_info.hit_item_arr = cvector_create(sizeof(struct HWBP_HIT_ITEM));
	mutex_lock(&g_hwbp_handle_info_mutex);
	cvector_pushback(g_hwbp_handle_info_arr, &hwbp_handle_info);
	mutex_unlock(&g_hwbp_handle_info_mutex);
	if (copy_to_user((void*)arg, &hwbp_handle_info.sample_hbp, 8)) {
		return -EINVAL;
	}
	return 0;
}

static long OnIoctlUninstProcessHwbp(unsigned long arg) {
	struct perf_event * sample_hbp = NULL;
	citerator iter;
	bool found = false;
	if (copy_from_user(&sample_hbp, (void __user *)arg, 8)) {
		return -EFAULT;
	}

	printk_debug(KERN_INFO "sample_hbp *:%px\n", sample_hbp);
	if(!sample_hbp) {
		return -EFAULT;
	}

	mutex_lock(&g_hwbp_handle_info_mutex);
	for (iter = cvector_begin(g_hwbp_handle_info_arr); iter != cvector_end(g_hwbp_handle_info_arr); iter = cvector_next(g_hwbp_handle_info_arr, iter)) {
		struct HWBP_HANDLE_INFO * hwbp_handle_info = (struct HWBP_HANDLE_INFO *)iter;
		if(hwbp_handle_info->sample_hbp == sample_hbp) {
			if(hwbp_handle_info->hit_item_arr) {
				cvector_destroy(hwbp_handle_info->hit_item_arr);
				hwbp_handle_info->hit_item_arr = NULL;
			}
			cvector_rm(g_hwbp_handle_info_arr, iter);
			found = true;
			break;
		}
	}
	mutex_unlock(&g_hwbp_handle_info_mutex);
	if(found) {
		x_unregister_hw_breakpoint(sample_hbp);
	}
	
	return 0;
}

static long OnIoctlSuspendProcessHwbp(unsigned long arg) {
	struct perf_event * sample_hbp = NULL;
	struct perf_event_attr new_instruction_attr;
	citerator iter;
	bool found = false;
	if (copy_from_user(&sample_hbp, (void __user *)arg, 8)) {
		return -EFAULT;
	}

	printk_debug(KERN_INFO "sample_hbp *:%px\n", sample_hbp);
	if(!sample_hbp) {
		return -EFAULT;
	}

	mutex_lock(&g_hwbp_handle_info_mutex);
	for (iter = cvector_begin(g_hwbp_handle_info_arr); iter != cvector_end(g_hwbp_handle_info_arr); iter = cvector_next(g_hwbp_handle_info_arr, iter)) {
		struct HWBP_HANDLE_INFO * hwbp_handle_info = (struct HWBP_HANDLE_INFO *)iter;
		if(hwbp_handle_info->sample_hbp == sample_hbp) {
			hwbp_handle_info->original_attr.disabled = 1;
			memcpy(&new_instruction_attr, &hwbp_handle_info->original_attr, sizeof(struct perf_event_attr));
			found = true;
			break;
		}
	}
	mutex_unlock(&g_hwbp_handle_info_mutex);
	if(found) {
		if(!x_modify_user_hw_breakpoint(sample_hbp, &new_instruction_attr)) {
			return 0;
		}
	}
	return -EFAULT;
}

static long OnIoctlResumeProcessHwbp(unsigned long arg) {
	struct perf_event * sample_hbp = NULL;
	struct perf_event_attr new_instruction_attr;
	citerator iter;
	bool found = false;
	if (copy_from_user(&sample_hbp, (void __user *)arg, 8)) {
		return -EFAULT;
	}

	printk_debug(KERN_INFO "sample_hbp *:%px\n", sample_hbp);
	if(!sample_hbp) {
		return -EFAULT;
	}

	mutex_lock(&g_hwbp_handle_info_mutex);
	for (iter = cvector_begin(g_hwbp_handle_info_arr); iter != cvector_end(g_hwbp_handle_info_arr); iter = cvector_next(g_hwbp_handle_info_arr, iter)) {
		struct HWBP_HANDLE_INFO * hwbp_handle_info = (struct HWBP_HANDLE_INFO *)iter;
		if(hwbp_handle_info->sample_hbp == sample_hbp) {
			hwbp_handle_info->original_attr.disabled = 0;
			memcpy(&new_instruction_attr, &hwbp_handle_info->original_attr, sizeof(struct perf_event_attr));
			found = true;
			break;
		}
	}
	mutex_unlock(&g_hwbp_handle_info_mutex);
	if(found) {
		if(!x_modify_user_hw_breakpoint(sample_hbp, &new_instruction_attr)) {
			return 0;
		}
	}
	return -EFAULT;
}

static long OnIoctlGetHwbpHitCount(unsigned long arg) {
	#pragma pack(1)
	struct buf_layout {
		struct perf_event *sample_hbp;
		uint64_t hit_total_count;
		uint64_t hit_item_arr_count;
	};
	#pragma pack()
	struct buf_layout user_data = {0};
	citerator iter;
	if (copy_from_user(&user_data.sample_hbp, (void __user *)arg, 8)) {
		return -EFAULT;
	}
	printk_debug(KERN_INFO "sample_hbp *:%px\n", user_data.sample_hbp);

	mutex_lock(&g_hwbp_handle_info_mutex);
	for (iter = cvector_begin(g_hwbp_handle_info_arr); iter != cvector_end(g_hwbp_handle_info_arr); iter = cvector_next(g_hwbp_handle_info_arr, iter)) {
		struct HWBP_HANDLE_INFO * hwbp_handle_info = (struct HWBP_HANDLE_INFO *)iter;
		if (hwbp_handle_info->sample_hbp == user_data.sample_hbp && hwbp_handle_info->hit_item_arr) {
			user_data.hit_total_count = hwbp_handle_info->hit_total_count;
			user_data.hit_item_arr_count = cvector_length(hwbp_handle_info->hit_item_arr);
			break;
		}
	}

	mutex_unlock(&g_hwbp_handle_info_mutex);
	
	printk_debug(KERN_INFO "user_data.hit_total_count:%zu\n", user_data.hit_total_count);
	if (copy_to_user((void*)arg, &user_data, 24)) {
		return -EINVAL;
	}
	return 0;
}

static long OnIoctlSetHookPc(unsigned long arg) {
	uint64_t pc = 0;
	if (copy_from_user(&pc, (void __user *)arg, 8)) {
		return -EFAULT;
	}
	printk_debug(KERN_INFO "pc:%px\n", pc);
	atomic64_set(&g_hook_pc, pc);
	return 0;
}

static ssize_t hwBreakpointProc_read(struct file* filp, char __user* buf, size_t size, loff_t* ppos) {
	struct perf_event * sample_hbp = NULL;
	ssize_t count = 0;
	size_t copy_pos;
	size_t end_pos;

	citerator iter;
	if (copy_from_user((void*)&sample_hbp, buf, 8)) {
		return -EFAULT;
	}
	printk_debug(KERN_INFO "sample_hbp *:%ld\n", sample_hbp);

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + size);

	mutex_lock(&g_hwbp_handle_info_mutex);
	for (iter = cvector_begin(g_hwbp_handle_info_arr); iter != cvector_end(g_hwbp_handle_info_arr); iter = cvector_next(g_hwbp_handle_info_arr, iter)) {
		struct HWBP_HANDLE_INFO * hwbp_handle_info = (struct HWBP_HANDLE_INFO *)iter;
		if (hwbp_handle_info->sample_hbp == sample_hbp && hwbp_handle_info->hit_item_arr) {
			citerator child;
			for (child = cvector_begin(hwbp_handle_info->hit_item_arr); child != cvector_end(hwbp_handle_info->hit_item_arr); child = cvector_next(hwbp_handle_info->hit_item_arr, child)) {
				struct HWBP_HIT_ITEM * hit_item = (struct HWBP_HIT_ITEM *)child;
				if (copy_pos >= end_pos) {
					break;
				}
				if (copy_to_user((void*)copy_pos, hit_item, sizeof(struct HWBP_HIT_ITEM))) {
					break;
				}
				copy_pos += sizeof(struct HWBP_HIT_ITEM);
				count++;
			}
			break;
		}
	}
	mutex_unlock(&g_hwbp_handle_info_mutex);
	return count;
}

static inline long DispatchCommand(unsigned int cmd, unsigned long arg) {
	switch (cmd) {
	case IOCTL_HWBP_OPEN_PROCESS: //打开进程
		printk_debug(KERN_INFO "IOCTL_HWBP_OPEN_PROCESS\n");
		return OnIoctlOpenProcess(arg);
	case IOCTL_HWBP_CLOSE_HANDLE: //关闭进程
		printk_debug(KERN_INFO "IOCTL_HWBP_CLOSE_HANDLE\n");
		return OnIoctlCloseProcess(arg);
	case IOCTL_HWBP_GET_NUM_BRPS: //获取CPU支持硬件执行断点的数量
		printk_debug(KERN_INFO "IOCTL_HWBP_GET_NUM_BRPS\n");
		return OnIoctlGetCpuNumBrps();
	case IOCTL_HWBP_GET_NUM_WRPS: //获取CPU支持硬件访问断点的数量
		printk_debug(KERN_INFO "IOCTL_HWBP_GET_NUM_WRPS\n");
		return OnIoctlGetCpuNumWrps();
	case IOCTL_HWBP_INST_PROCESS_HWBP: //设置进程硬件断点
		printk_debug(KERN_INFO "IOCTL_HWBP_INST_PROCESS_HWBP\n");
		return OnIoctlInstProcessHwbp(arg);
	case IOCTL_HWBP_UNINST_PROCESS_HWBP: //删除进程硬件断点
		printk_debug(KERN_INFO "IOCTL_HWBP_UNINST_PROCESS_HWBP\n");
		return OnIoctlUninstProcessHwbp(arg);
	case IOCTL_HWBP_SUSPEND_PROCESS_HWBP: //暂停进程硬件断点
		printk_debug(KERN_INFO "IOCTL_HWBP_SUSPEND_PROCESS_HWBP\n");
		return OnIoctlSuspendProcessHwbp(arg);
	case IOCTL_HWBP_RESUME_PROCESS_HWBP: //恢复进程硬件断点
		printk_debug(KERN_INFO "IOCTL_HWBP_RESUME_PROCESS_HWBP\n");
		return OnIoctlResumeProcessHwbp(arg);
	case IOCTL_HWBP_GET_HWBP_HIT_COUNT: //获取硬件断点命中地址数量
		printk_debug(KERN_INFO "IOCTL_HWBP_GET_HWBP_HIT_COUNT\n");
		return OnIoctlGetHwbpHitCount(arg);
	case IOCTL_HWBP_SET_HOOK_PC:
		printk_debug(KERN_INFO "IOCTL_HWBP_SET_HOOK_PC\n");
		return OnIoctlSetHookPc(arg);
	default:
		return -EINVAL;
	}
	return -EINVAL;
}


//long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
//long (*compat_ioctl) (struct file *, unsigned int cmd, unsigned long arg)
static long hwBreakpointProc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	return DispatchCommand(cmd, arg);
}

static int hwBreakpointProc_open(struct inode *inode, struct file *filp) {
	g_hwBreakpointProc_devp->cur_dev_open_count++;
	if (g_hwBreakpointProc_devp->cur_dev_open_count >= 2) {
		if (!g_hwBreakpointProc_devp->is_already_hide_dev_file) {
			g_hwBreakpointProc_devp->is_already_hide_dev_file = true;
			device_destroy(g_Class_devp, g_hwBreakpointProc_devno);
			class_destroy(g_Class_devp);
		}
	}
	return 0;
}

static loff_t hwBreakpointProc_llseek(struct file* filp, loff_t offset, int orig) {
	unsigned int cmd = 0;
	printk_debug("hwBreakpointProc llseek offset:%zd\n", (ssize_t)offset);
	if (!!copy_from_user((void*)&cmd, (void*)offset, sizeof(unsigned int))) {
		return -EINVAL;
	}
	printk_debug("hwBreakpointProc llseek cmd:%u\n", cmd);
	return DispatchCommand(cmd, offset + sizeof(unsigned int));
}

static ssize_t hwBreakpointProc_write(struct file* filp, const char __user* buf, size_t size, loff_t *ppos) {
	return 0;
}

static void clean_hwbp(void) {
	citerator iter;
	cvector wait_unregister_bp_arr = cvector_create(sizeof(struct perf_event *));
	if(!wait_unregister_bp_arr || !g_hwbp_handle_info_arr) {
		return;
	}
	mutex_lock(&g_hwbp_handle_info_mutex);
	for (iter = cvector_begin(g_hwbp_handle_info_arr); iter != cvector_end(g_hwbp_handle_info_arr); iter = cvector_next(g_hwbp_handle_info_arr, iter)) {
		struct HWBP_HANDLE_INFO * hwbp_handle_info = (struct HWBP_HANDLE_INFO *)iter;
		if(hwbp_handle_info->sample_hbp) {
			cvector_pushback(wait_unregister_bp_arr, &hwbp_handle_info->sample_hbp);
			hwbp_handle_info->sample_hbp = NULL;
		}
		if(hwbp_handle_info->hit_item_arr) {
			cvector_destroy(hwbp_handle_info->hit_item_arr);
			hwbp_handle_info->hit_item_arr = NULL;
		}
	}
	cvector_destroy(g_hwbp_handle_info_arr);
	g_hwbp_handle_info_arr = NULL;
	mutex_unlock(&g_hwbp_handle_info_mutex);
	for (iter = cvector_begin(wait_unregister_bp_arr); iter != cvector_end(wait_unregister_bp_arr); iter = cvector_next(wait_unregister_bp_arr, iter)) {
	  struct perf_event * bp = *(struct perf_event **)iter;
	  x_unregister_hw_breakpoint(bp);
	}
	cvector_destroy(wait_unregister_bp_arr);
}

static int hwBreakpointProc_release(struct inode *inode, struct file *filp) {
	if (g_hwBreakpointProc_devp->cur_dev_open_count > 0) {
		g_hwBreakpointProc_devp->cur_dev_open_count--;
	}
	if (g_hwBreakpointProc_devp->cur_dev_open_count < 1) {
		if (g_hwBreakpointProc_devp->is_already_hide_dev_file) {
			g_hwBreakpointProc_devp->is_already_hide_dev_file = false;

			g_Class_devp = class_create(THIS_MODULE, DEV_FILENAME);
			device_create(g_Class_devp, NULL, g_hwBreakpointProc_devno, NULL, "%s", DEV_FILENAME);
		}
	}

	clean_hwbp();
	mutex_lock(&g_hwbp_handle_info_mutex);
	g_hwbp_handle_info_arr = cvector_create(sizeof(struct HWBP_HANDLE_INFO));
	mutex_unlock(&g_hwbp_handle_info_mutex);
	return 0;
}

static const struct file_operations hwBreakpointProc_fops =
{
  .owner = THIS_MODULE,
  .open = hwBreakpointProc_open,
  .release = hwBreakpointProc_release,
  .read = hwBreakpointProc_read,
  .write = hwBreakpointProc_write,
  .llseek = hwBreakpointProc_llseek,

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
	.ioctl = hwBreakpointProc_ioctl,
#else
	.compat_ioctl = hwBreakpointProc_ioctl,
	.unlocked_ioctl = hwBreakpointProc_ioctl,
#endif
};

#ifdef CONFIG_MODULE_GUIDE_ENTRY
static
#endif
int __init hwBreakpointProc_dev_init(void) {
	int result;
	int err;

#ifdef CONFIG_KALLSYMS_LOOKUP_NAME
	if(!init_kallsyms_lookup()) {
		printk(KERN_EMERG "init_kallsyms_lookup failed\n");
		return -EBADF;
	}
#endif

	g_hwbp_handle_info_arr = cvector_create(sizeof(struct HWBP_HANDLE_INFO));
	if(!g_hwbp_handle_info_arr) {
		printk(KERN_EMERG "cvector_create failed\n");
		return -ENOMEM;
	}

	mutex_init(&g_hwbp_handle_info_mutex);

#ifdef CONFIG_ANTI_PTRACE_DETECTION_MODE
	start_anti_ptrace_detection(&g_hwbp_handle_info_mutex, &g_hwbp_handle_info_arr);
#endif

	result = alloc_chrdev_region(&g_hwBreakpointProc_devno, 0, 1, DEV_FILENAME);
	g_hwBreakpointProc_major = MAJOR(g_hwBreakpointProc_devno);

	if (result < 0) {
		printk(KERN_EMERG "hwBreakpointProc alloc_chrdev_region failed %d\n", result);
		return result;
	}

	g_hwBreakpointProc_devp = __kmalloc(sizeof(struct hwBreakpointProcDev), GFP_KERNEL);
	if (!g_hwBreakpointProc_devp) {
		result = -ENOMEM;
		goto _fail;
	}
	memset(g_hwBreakpointProc_devp, 0, sizeof(struct hwBreakpointProcDev));

	g_hwBreakpointProc_devp->pcdev = __kmalloc(sizeof(struct cdev) * 3/*大些兼容性强*/, GFP_KERNEL);
	cdev_init(g_hwBreakpointProc_devp->pcdev, &hwBreakpointProc_fops);
	g_hwBreakpointProc_devp->pcdev->owner = THIS_MODULE;
	g_hwBreakpointProc_devp->pcdev->ops = &hwBreakpointProc_fops;
	err = cdev_add(g_hwBreakpointProc_devp->pcdev, g_hwBreakpointProc_devno, 1);
	if (err) {
		printk(KERN_EMERG "Error in cdev_add()\n");
		result = -EFAULT;
		goto _fail;
	}

	g_Class_devp = class_create(THIS_MODULE, DEV_FILENAME);
	device_create(g_Class_devp, NULL, g_hwBreakpointProc_devno, NULL, "%s", DEV_FILENAME);

#ifdef DEBUG_PRINTK
	printk(KERN_EMERG "Hello, %s debug\n", DEV_FILENAME);
#else
	printk(KERN_EMERG "Hello, %s\n", DEV_FILENAME);
#endif
	return 0;

_fail:
	unregister_chrdev_region(g_hwBreakpointProc_devno, 1);
	return result;
}

#ifdef CONFIG_MODULE_GUIDE_ENTRY
static
#endif
void __exit hwBreakpointProc_dev_exit(void) {
	
#ifdef CONFIG_ANTI_PTRACE_DETECTION_MODE
	stop_anti_ptrace_detection();
#endif

	clean_hwbp();
	
	mutex_destroy(&g_hwbp_handle_info_mutex);
	
	device_destroy(g_Class_devp, g_hwBreakpointProc_devno);
	class_destroy(g_Class_devp);

	cdev_del(g_hwBreakpointProc_devp->pcdev);
	kfree(g_hwBreakpointProc_devp->pcdev);
	kfree(g_hwBreakpointProc_devp);
	unregister_chrdev_region(g_hwBreakpointProc_devno, 1);
	printk(KERN_EMERG "Goodbye, %s\n", DEV_FILENAME);
}

#ifndef CONFIG_MODULE_GUIDE_ENTRY
//Hook:__cfi_check_fn
unsigned char* __check_(unsigned char* result, void *ptr, void *diag)
{
	printk_debug(KERN_EMERG "my__cfi_check_fn!!!\n");
	return result;
}

//Hook:__cfi_check_fail
unsigned char * __check_fail_(unsigned char *result)
{
	printk_debug(KERN_EMERG "my__cfi_check_fail!!!\n");
	return result;
}
#endif

unsigned long __stack_chk_guard;

#ifdef CONFIG_MODULE_GUIDE_ENTRY
module_init(hwBreakpointProc_dev_init);
module_exit(hwBreakpointProc_dev_exit);
#endif

MODULE_AUTHOR("Linux");
MODULE_DESCRIPTION("Linux default module");
MODULE_LICENSE("GPL");
