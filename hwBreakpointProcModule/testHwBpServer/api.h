#ifndef API_H_
#define API_H_

#include <stdint.h>
#include <pthread.h>
#include <sys/queue.h>
#include "Global.h"

#include "porthelp.h"
#include "hwbpserver.h"
#ifdef __ANDROID__
#include<android/log.h>
#endif
/*

#if defined(__arm__) || defined(__ANDROID__)
#include <linux/user.h>
#else
#include <sys/user.h>
#endif
*/

#ifdef HAS_LINUX_USER_H
#include <linux/user.h>
#else
#include <sys/user.h>
#endif

#include <string>


/*
struct  CeOpenProcess
{
	int pid;
	uint64_t driverProcessHandle;
};
*/

BOOL GetProcessTask(int pid, std::vector<int> & vOutput);
/*
void CloseHandle(HANDLE h);
HANDLE OpenProcess(DWORD pid);
*/
void ProcessAddProcessHwBp(AddProcessHwBpInfo &params,
	int &allTaskCount, int &insHwBpSuccessTaskCount, std::vector<struct USER_HIT_INFO> &vHit);
int ProcessSetHwBpHitConditions(struct HIT_CONDITIONS params);
#ifdef __ANDROID__
#define LOG_TAG "HWSERVER"
#define LOGD(fmt, args...) __android_log_vprint(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#endif

#endif /* API_H_ */
