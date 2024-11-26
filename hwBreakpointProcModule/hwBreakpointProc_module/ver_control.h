#ifndef VER_CONTROL_H_
#define VER_CONTROL_H_
#include <linux/version.h>
#define DEV_FILENAME "hwbpProc3"

#define CONFIG_MODULE_GUIDE_ENTRY

// 调试打印模式
//#define CONFIG_DEBUG_PRINTK

// 动态寻址模式
#define CONFIG_KALLSYMS_LOOKUP_NAME

// 精准命中记录模式
#define CONFIG_MODIFY_HIT_NEXT_MODE

// 反PTRACE侦测模式
#define CONFIG_ANTI_PTRACE_DETECTION_MODE

#ifdef CONFIG_DEBUG_PRINTK
#define printk_debug printk
#else
static inline void printk_debug(char *fmt, ...) {}
#endif

#endif /* VER_CONTROL_H_ */
