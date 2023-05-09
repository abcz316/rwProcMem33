#ifndef API_PROXY_H_
#define API_PROXY_H_
#include "ver_control.h"
#include "linux_kernel_api/linux_kernel_api_4_19_81.h"
#include "linux_kernel_api/linux_kernel_api_5_4_61.h"
#include "linux_kernel_api/linux_kernel_api_5_10_43.h"
#include <linux/ctype.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>

//声明
//////////////////////////////////////////////////////////////////////////
MY_STATIC inline int x_atoi(const char arr[]);
MY_STATIC inline unsigned long x_copy_from_user(void *to, const void __user *from, unsigned long n);
MY_STATIC inline unsigned long x_copy_to_user(void __user *to, const void *from, unsigned long n);
MY_STATIC struct task_struct* x_get_current(void);

//实现
//////////////////////////////////////////////////////////////////////////
MY_STATIC inline int x_atoi(const char arr[]) {
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

MY_STATIC inline unsigned long x_copy_from_user(void *to, const void __user *from, unsigned long n) {
#ifdef CONFIG_DIRECT_API_USER_COPY
	unsigned long __arch_copy_from_user(void* to, const void __user * from, unsigned long n);
	return __arch_copy_from_user(to, from, n);
#else
	return copy_from_user(to, from, n);
#endif
}
MY_STATIC inline unsigned long x_copy_to_user(void __user *to, const void *from, unsigned long n) {
#ifdef CONFIG_DIRECT_API_USER_COPY
	unsigned long __arch_copy_to_user(void __user * to, const void* from, unsigned long n);
	return __arch_copy_to_user(to, from, n);
#else
	return copy_to_user(to, from, n);
#endif
}

MY_STATIC __always_inline struct task_struct *x_get_current(void)
{
	unsigned long sp_el0;
	asm ("mrs %0, sp_el0" : "=r" (sp_el0));
	return (struct task_struct *)sp_el0;
}

#endif /* API_PROXY_H_ */


