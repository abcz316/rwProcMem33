#ifndef VERSION_CONTROL_H_
#define VERSION_CONTROL_H_
#include <linux/version.h>
#define DEV_FILENAME "rwProcMem37" //当前驱动DEV文件名

//独立内核模块入口模式
#define CONFIG_MODULE_GUIDE_ENTRY

//启用读取pagemap文件来计算物理内存的地址
//#define CONFIG_USE_PAGEMAP_FILE

//打印内核调试信息
//#define CONFIG_DEBUG_PRINTK

//是否启用匿名函数名模式
#define CONFIG_ANONYMOUS_FUNC_NAME_MODE

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif
#ifndef MY_LINUX_VERSION_CODE 
//#define MY_LINUX_VERSION_CODE KERNEL_VERSION(3,10,0)
//#define MY_LINUX_VERSION_CODE KERNEL_VERSION(3,10,84)
//#define MY_LINUX_VERSION_CODE KERNEL_VERSION(3,18,71)
//#define MY_LINUX_VERSION_CODE KERNEL_VERSION(3,18,140)
//#define MY_LINUX_VERSION_CODE KERNEL_VERSION(4,4,21)
//#define MY_LINUX_VERSION_CODE KERNEL_VERSION(4,4,78)
//#define MY_LINUX_VERSION_CODE KERNEL_VERSION(4,4,153)
//#define MY_LINUX_VERSION_CODE KERNEL_VERSION(4,4,192)
//#define MY_LINUX_VERSION_CODE KERNEL_VERSION(4,9,112)
//#define MY_LINUX_VERSION_CODE KERNEL_VERSION(4,9,186)
//#define MY_LINUX_VERSION_CODE KERNEL_VERSION(4,14,83)
//#define MY_LINUX_VERSION_CODE KERNEL_VERSION(4,14,117)
//#define MY_LINUX_VERSION_CODE KERNEL_VERSION(4,14,141)
#define MY_LINUX_VERSION_CODE KERNEL_VERSION(4,19,81)
//#define MY_LINUX_VERSION_CODE KERNEL_VERSION(5,4,61)
//#define MY_LINUX_VERSION_CODE KERNEL_VERSION(5,10,43)
#endif


#if MY_LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,83)
#include <linux/sched/task.h>
#include <linux/sched/mm.h>
#include <linux/sched/signal.h>
#endif



#if MY_LINUX_VERSION_CODE <= KERNEL_VERSION(4,4,192)
#define FILE_OP_DIR_ITER iterate
#endif

#ifndef FILE_OP_DIR_ITER
#define FILE_OP_DIR_ITER iterate_shared
#endif



#if MY_LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,84)
#define KUID_T_VALUE
#define KGID_T_VALUE
#else
#define KUID_T_VALUE .val
#define KGID_T_VALUE .val
#endif


#ifdef CONFIG_ANONYMOUS_FUNC_NAME_MODE
#define MY_STATIC static
#else
#define MY_STATIC
#endif

#ifdef CONFIG_DEBUG_PRINTK
#define printk_debug printk
#else
static inline void printk_debug(char *fmt, ...) {}
#endif

#endif /* VERSION_CONTROL_H_ */