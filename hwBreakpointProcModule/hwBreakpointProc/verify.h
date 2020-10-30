#ifndef VERIFY_H_
#define VERIFY_H_
#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/random.h>
#include <linux/timer.h>
#include "version_control.h"
#include "crc64.h"

static size_t g_rwProcMemVerifyPassed = 0;
static int g_rwProcMemVerifyInitSuccess = 0;
static char g_rwProcMemVerifyQuestion[128] = { 0 };
static size_t g_rwProcMemVerifyKey = 0;
static size_t g_rwProcMemVerifyHiddenProtect = 0;
static struct timer_list g_rwProcMemVerifyTimer;

static inline size_t is_verify_passed(void)
{
	//return g_rwProcMemVerifyPassed;
	return true;
}

static inline void init_verify(void)
{
	return;
}

static void irq_verify_timer_function(unsigned long data)
{
	//定时重新验证
	g_rwProcMemVerifyPassed = 0;
	g_rwProcMemVerifyHiddenProtect = 0;
	memset(&g_rwProcMemVerifyQuestion, 0, sizeof(g_rwProcMemVerifyQuestion));
	g_rwProcMemVerifyKey = 0;
	mod_timer(&g_rwProcMemVerifyTimer, jiffies + HZ * 43200); //12小时
}
static inline void start_verify_timer(void)
{
	//启动定时验证时钟，防止长时间无验证
#ifndef timer_setup
	init_timer(&g_rwProcMemVerifyTimer);
	g_rwProcMemVerifyTimer.expires = jiffies + HZ * 43200; //12小时
	g_rwProcMemVerifyTimer.data = (unsigned long)0;
	g_rwProcMemVerifyTimer.function = irq_verify_timer_function;
	add_timer(&g_rwProcMemVerifyTimer);
#else
	g_rwProcMemVerifyTimer.expires = jiffies + HZ * 43200; //12小时
	timer_setup(&g_rwProcMemVerifyTimer, irq_verify_timer_function, 0);
	add_timer(&g_rwProcMemVerifyTimer);
#endif
}
static inline void close_verify_timer(void)
{
	del_timer(&g_rwProcMemVerifyTimer);
}
static inline size_t set_verify_key(size_t key)
{
	g_rwProcMemVerifyKey = key;
	g_rwProcMemVerifyPassed = 1;
	return 0;
}

#endif /* VERIFY_H_ */