#ifndef HWBPSERVER_H_
#define HWBPSERVER_H_

#include <stdint.h>
#include <sys/types.h>
#include <vector>

#ifdef _linux_
#include "porthelp.h"
#include "Global.h"
#endif

#define CMD_GETVERSION 0
#define CMD_CLOSECONNECTION 1
#define CMD_TERMINATESERVER 2
#define CMD_ADDPROCESSHWBP 3

//extern char *versionstring;
#pragma pack(1)
struct HwBpVersion {
	int version = 0;
	unsigned char stringsize = 0;
	//append the versionstring
};
struct InstProcessHwBpInfo {
	uint64_t pid = 0;
	uint64_t address = 0;
	uint32_t hwBpAddrLen = 0;
	uint32_t hwBpAddrType = 0;
	uint32_t hwBpThreadType = 0;
	uint32_t hwBpKeepTimeMs = 0;
};

struct InstProcessHwBpResultChild {
	uint64_t taskId = 0;
	uint64_t address = 0;
	uint64_t hitTotalCount = 0;
	std::vector<struct HW_HIT_ITEM> vHitItem;
};

struct InstProcessHwBpResult {
	uint32_t allTaskCount = 0;
	uint32_t hwbpInstalledCount = 0;
	std::vector<struct InstProcessHwBpResultChild> vThreadHit;
};
#pragma pack()


#endif /* HWBPSERVER_H_ */