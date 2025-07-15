#ifndef _KALLSYMS_LOOKUP_API_H_
#define _KALLSYMS_LOOKUP_API_H_
#include "ver_control.h"
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/kallsyms.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <linux/kprobes.h>

static unsigned long (*kallsyms_lookup_name_sym)(const char *name);
static struct perf_event* (*register_user_hw_breakpoint_sym)(struct perf_event_attr *attr, perf_overflow_handler_t triggered, void *context, struct task_struct *tsk);
static void (*unregister_hw_breakpoint_sym)(struct perf_event *bp);
#ifdef CONFIG_MODIFY_HIT_NEXT_MODE
static int (*modify_user_hw_breakpoint_sym)(struct perf_event *bp, struct perf_event_attr *attr);
#endif

static int _kallsyms_lookup_kprobe(struct kprobe *p, struct pt_regs *regs) { return 0; }
static unsigned long get_kallsyms_func(void) {
	int ret;
	unsigned long addr = 0;
	struct kprobe probe = {0};
	probe.pre_handler = _kallsyms_lookup_kprobe;
	probe.symbol_name = "kallsyms_lookup_name";
	ret = register_kprobe(&probe);
	if (ret == 0) {
		addr = (unsigned long)probe.addr;
		printk_debug(KERN_EMERG "get_kallsyms_func(kallsyms_lookup_name):%px\n", addr);
		unregister_kprobe(&probe);
	}
	return addr;
}

static unsigned long generic_kallsyms_lookup_name(const char *name) {
	if (!kallsyms_lookup_name_sym) {
			kallsyms_lookup_name_sym = (void *)get_kallsyms_func();
			printk_debug(KERN_EMERG "get_kallsyms_func:%px\n", kallsyms_lookup_name_sym);
			if(!kallsyms_lookup_name_sym)
					return 0;
	}
	return kallsyms_lookup_name_sym(name);
}

static bool init_kallsyms_lookup(void) {
	register_user_hw_breakpoint_sym = (void *)generic_kallsyms_lookup_name("register_user_hw_breakpoint");
	printk_debug(KERN_EMERG "register_user_hw_breakpoint_sym:%px\n", register_user_hw_breakpoint_sym);
	if(!register_user_hw_breakpoint_sym) { return false; }

	unregister_hw_breakpoint_sym = (void *)generic_kallsyms_lookup_name("unregister_hw_breakpoint");
	printk_debug(KERN_EMERG "unregister_hw_breakpoint_sym:%px\n", unregister_hw_breakpoint_sym);
	if(!unregister_hw_breakpoint_sym) { return false; }

#ifdef CONFIG_MODIFY_HIT_NEXT_MODE
	modify_user_hw_breakpoint_sym = (void *)generic_kallsyms_lookup_name("modify_user_hw_breakpoint");
	printk_debug(KERN_EMERG "modify_user_hw_breakpoint_sym:%px\n", modify_user_hw_breakpoint_sym);
	if(!modify_user_hw_breakpoint_sym) { return false; }
#endif

	return true;
}
#endif
