#ifndef VERSION_CONTROL_H_
#define VERSION_CONTROL_H_

//当前驱动版本号
#define DEV_FILENAME "hwBreakpointProc1"

//#define DEBUG_PRINTK

//#define LINUX_VERSION_3_10_0
#define LINUX_VERSION_3_10_84
//#define LINUX_VERSION_3_18_71
//#define LINUX_VERSION_4_4_21
//#define LINUX_VERSION_4_4_78
//#define LINUX_VERSION_4_4_153
//#define LINUX_VERSION_4_4_192
//#define LINUX_VERSION_4_9_112
//#define LINUX_VERSION_4_14_83
//#define LINUX_VERSION_4_14_117



//是否启用验证KEY系统（需要打开WIFI硬件模块才能正常工作）
#define CONFIG_VERIFY
//是否打印内核调试日志
//#define CONFIG_DEBUG_PRINTK



#ifdef LINUX_VERSION_4_14_117
#include <linux/sched/task.h>
#include <linux/sched/mm.h>
#endif



#ifdef DEBUG_PRINTK
#define printk_debug printk
#else
static inline void printk_debug(const char *fmt, ...)
{
	return;
}
#endif

#endif /* VERSION_CONTROL_H_ */