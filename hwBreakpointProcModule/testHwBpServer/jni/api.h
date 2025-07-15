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


BOOL GetProcessTask(int pid, std::vector<int> & vOutput);
void ProcessInstProcessHwBp(const struct InstProcessHwBpInfo& input, struct InstProcessHwBpResult & output);
#ifdef __ANDROID__
#define LOG_TAG "HWSERVER"
#define LOGD(fmt, args...) __android_log_vprint(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#endif

#endif /* API_H_ */
