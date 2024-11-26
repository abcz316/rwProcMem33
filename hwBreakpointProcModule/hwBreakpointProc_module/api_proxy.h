#ifndef _API_PROXY_H_
#define _API_PROXY_H_
#include "ver_control.h"

#ifdef CONFIG_KALLSYMS_LOOKUP_NAME
#include "kallsyms_lookup_api.h"
static struct perf_event* x_register_user_hw_breakpoint(struct perf_event_attr *attr, perf_overflow_handler_t triggered, void *context, struct task_struct *tsk) {
	return register_user_hw_breakpoint_sym(attr, triggered, context, tsk);
}

static void x_unregister_hw_breakpoint(struct perf_event *bp) {
	unregister_hw_breakpoint_sym(bp);
}

static int x_modify_user_hw_breakpoint(struct perf_event *bp, struct perf_event_attr *attr) {
	return modify_user_hw_breakpoint_sym(bp, attr);
}
#else
static struct perf_event* x_register_user_hw_breakpoint(struct perf_event_attr *attr, perf_overflow_handler_t triggered, void *context, struct task_struct *tsk) {
	return register_user_hw_breakpoint(attr, triggered, context, tsk);
}

static void x_unregister_hw_breakpoint(struct perf_event *bp) {
	unregister_hw_breakpoint(bp);
}

static int x_modify_user_hw_breakpoint(struct perf_event *bp, struct perf_event_attr *attr) {
	return modify_user_hw_breakpoint(bp, attr);
}

#endif

#ifdef CONFIG_ANTI_PTRACE_DETECTION_MODE
#include "anti_ptrace_detection.h"
#endif
#endif
