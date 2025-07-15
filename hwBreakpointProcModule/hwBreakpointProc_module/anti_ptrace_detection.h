#ifndef _ANTI_PTRACE_DETECTION_H_
#define _ANTI_PTRACE_DETECTION_H_
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/hw_breakpoint.h>
#include <linux/kprobes.h>

#define PTRACE_GETREGSET   0x4204
#define NT_ARM_HW_BREAK	0x402		/* ARM hardware breakpoint registers */
#define NT_ARM_HW_WATCH	0x403		/* ARM hardware watchpoint registers */

struct hook_ptrace_data {
    struct iovec iov;
};

static struct mutex *g_p_hwbp_handle_info_mutex = NULL;
static cvector *g_p_hwbp_handle_info_arr = NULL;

static bool is_my_hwbp_handle_addr(size_t addr) {
	citerator iter;
	bool found = false;
	if(addr == 0) {
		return found;
	}
	mutex_lock(g_p_hwbp_handle_info_mutex);
	for (iter = cvector_begin(*g_p_hwbp_handle_info_arr); iter != cvector_end(*g_p_hwbp_handle_info_arr); iter = cvector_next(*g_p_hwbp_handle_info_arr, iter)) {
		struct HWBP_HANDLE_INFO * hwbp_handle_info = (struct HWBP_HANDLE_INFO *)iter;
		if(hwbp_handle_info->original_attr.bp_addr == addr) {
			found = true;
			break;
		}
	}
	mutex_unlock(g_p_hwbp_handle_info_mutex);
	return found;
}

static int entry_ptrace_handler(struct kretprobe_instance *ri, struct pt_regs *regs) {
    long request = regs->regs[1];
    unsigned long addr = (unsigned long)regs->regs[2];
    struct hook_ptrace_data *data = (struct hook_ptrace_data *)ri->data;
    data->iov.iov_base = 0;
    data->iov.iov_len = 0;
    printk_debug(KERN_INFO "entry_ptrace_handler called with request: %lx, addr: %lx\n", request, addr);
    if (request == PTRACE_GETREGSET && (addr == NT_ARM_HW_WATCH || addr == NT_ARM_HW_BREAK)) {
        unsigned long iov_user_ptr = regs->regs[3];
        printk_debug(KERN_INFO "entry_ptrace_handler called with request: %lx, addr: %lx, iov_user_ptr: %lx\n", request, addr, iov_user_ptr);
        if(!iov_user_ptr) {
            return 0;
        }
        if (x_copy_from_user(&data->iov, (struct iovec __user *)iov_user_ptr, sizeof(struct iovec)) != 0) {
            printk_debug(KERN_INFO "Failed to copy iovec from user space\n");
            return 0;
        }
        printk_debug(KERN_INFO "entry_ptrace_handler iov_base: %lx, iov_len %ld\n", data->iov.iov_base, data->iov.iov_len);
    }
    return 0;
}
static int ret_ptrace_handler(struct kretprobe_instance *ri, struct pt_regs *regs) {
    unsigned long retval = regs_return_value(regs);
    struct hook_ptrace_data *data = (struct hook_ptrace_data *)ri->data;
    struct user_hwdebug_state old_hw_state;
    struct user_hwdebug_state new_hw_state;
    size_t copy_size;
    int i = 0, y = 0;
    printk_debug(KERN_INFO "ret_ptrace_handler called with retval: %lx, iov_base: %lx, iov_len %ld\n", retval, data->iov.iov_base, data->iov.iov_len);
    if (!data->iov.iov_base || !data->iov.iov_len) {
        return 0;
    }
    
    // Check if the buffer of the IoV is readable and writable
    if (!access_ok((void __user *)data->iov.iov_base, data->iov.iov_len)) {
        printk_debug(KERN_INFO "User buffer is not accessible\n");
        return 0;
    }
    copy_size = min(data->iov.iov_len, sizeof(struct user_hwdebug_state));
    if (x_copy_from_user(&old_hw_state, (void __user *)data->iov.iov_base, copy_size) != 0) {
        printk_debug(KERN_INFO "Failed to copy old_hw_state from user buffer\n");
        return 0;
    }
    // After x_copy_from_user
    printk_debug(KERN_INFO "Original old_hw_state.dbg_info: %u, size %ld\n", old_hw_state.dbg_info, copy_size);
    for (i = 0; i < 16; i++) {
        printk_debug(KERN_INFO "Reg %d: addr=%llu, ctrl=%u\n", i, old_hw_state.dbg_regs[i].addr, old_hw_state.dbg_regs[i].ctrl);
    }
    // Clear the dbd_regs array
    memcpy(&new_hw_state, &old_hw_state, sizeof(new_hw_state));
    memset(new_hw_state.dbg_regs, 0x00, sizeof(new_hw_state.dbg_regs));

    printk_debug(KERN_INFO "After memset:\n");
    for (i = 0; i < sizeof(old_hw_state.dbg_regs) / sizeof(old_hw_state.dbg_regs[0]); i++) {
        if(!is_my_hwbp_handle_addr(old_hw_state.dbg_regs[i].addr)) {
            memcpy(&new_hw_state.dbg_regs[y++], &old_hw_state.dbg_regs[i], sizeof(old_hw_state.dbg_regs[i]));
        }
    }

    printk_debug(KERN_INFO "After memset:\n");
    for (i = 0; i < 16; i++) {
        printk_debug(KERN_INFO "Reg %d: addr=%llu, ctrl=%u\n", i, new_hw_state.dbg_regs[i].addr, new_hw_state.dbg_regs[i].ctrl);
    }

    // Copy the modified hw_ste back to the buffer in user space
    if (x_copy_to_user((void __user *)data->iov.iov_base, &new_hw_state, copy_size) != 0) {
        printk_debug(KERN_INFO "Failed to copy modified new_hw_state back to user buffer\n");
    } else {
        printk_debug(KERN_INFO "Successfully cleared dbg_regs in user_hwdebug_state\n");
    }
    return 0;
}


static struct kretprobe kretp_ptrace = {
    .kp.symbol_name    = "arch_ptrace",
    .data_size  = sizeof(struct hook_ptrace_data),
    .entry_handler    = entry_ptrace_handler,
    .handler    = ret_ptrace_handler,
    .maxactive  = 20,
};

static bool start_anti_ptrace_detection(struct mutex *p_hwbp_handle_info_mutex, cvector *p_hwbp_handle_info_arr) {
    int ret = 0;
    g_p_hwbp_handle_info_mutex = p_hwbp_handle_info_mutex;
    g_p_hwbp_handle_info_arr = p_hwbp_handle_info_arr;
    if(!g_p_hwbp_handle_info_mutex || !g_p_hwbp_handle_info_arr) {
        printk_debug(KERN_INFO "start_anti_ptrace_detection param error\n");
        return false;
    }
    ret = register_kretprobe(&kretp_ptrace);
    if (ret < 0) {
        printk_debug(KERN_INFO "register_kretprobe failed, returned %d\n", ret);
        return false;
    }
    printk_debug(KERN_INFO "kretprobe at %s registered, addr: %lx\n", kretp_ptrace.kp.symbol_name, kretp_ptrace.kp.addr);
	return true;
}


static void stop_anti_ptrace_detection(void) {
    if(kretp_ptrace.kp.addr) {
        unregister_kretprobe(&kretp_ptrace);
        printk_debug(KERN_INFO "kretprobe unregistered\n");
    }
}

#endif
