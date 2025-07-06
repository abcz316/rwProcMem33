#ifndef API_PROXY_H_
#define API_PROXY_H_
#include "ver_control.h"
#include "linux_kernel_api.h"
#include <linux/ctype.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

//声明
//////////////////////////////////////////////////////////////////////////
static inline int x_atoi(const char arr[]);
static inline bool x_isdigit(char c);
static inline struct task_struct* x_get_current(void);
static inline void * x_kmalloc(size_t size, gfp_t flags);
static inline unsigned long x_copy_from_user(void *to, const void __user *from, unsigned long n);
static inline unsigned long x_copy_to_user(void __user *to, const void *from, unsigned long n);

//实现
//////////////////////////////////////////////////////////////////////////
static inline bool x_isdigit(char c) { return (unsigned)(c - '0') < 10; }
static inline int x_atoi(const char arr[]) {
	int index = 0;
	int flag = 1;
	int num = 0;

	if (arr == NULL) { return -1; }
	while (isspace(arr[index])) { index++; }
	if (arr[index] == '-') { flag = -1; }
	if (arr[index] == '-' || arr[index] == '+') { index++; }
	while (arr[index] >= '0' && arr[index] <= '9') { num = num * 10 + arr[index] - '0';	index++; }
	return flag * num;
}

static struct task_struct *x_get_current(void) {
	unsigned long sp_el0;
	asm ("mrs %0, sp_el0" : "=r" (sp_el0));
	return (struct task_struct *)sp_el0;
}

static void * x_kmalloc(size_t size, gfp_t flags) {
	return __kmalloc(size, flags);
}

static unsigned long x_copy_from_user(void *to, const void __user *from, unsigned long n) {
	return __arch_copy_from_user(to, from, n);
}

static unsigned long x_copy_to_user(void __user *to, const void *from, unsigned long n) {
	return __arch_copy_to_user(to, from, n);
}
#endif /* API_PROXY_H_ */


