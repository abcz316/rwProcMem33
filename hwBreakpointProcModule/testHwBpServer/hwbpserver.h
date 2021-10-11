#ifndef HWBPSERVER_H_
#define HWBPSERVER_H_

#include <stdint.h>
#include <sys/types.h>
#ifdef linux
#include "porthelp.h"
#include "Global.h"
#endif


#define CMD_GETVERSION 0
#define CMD_CLOSECONNECTION 1
#define CMD_TERMINATESERVER 2
#define CMD_OPENPROCESS 3
#define CMD_CLOSEHANDLE 4
#define CMD_SETHITCONDITIONS 5
#define CMD_ADDPROCESSHWBP 6



//extern char *versionstring;
#pragma pack(1)
struct HwBpVersion {
	int version;
	unsigned char stringsize;
	//append the versionstring
};
struct AddProcessHwBpInfo {
	uint32_t pid;
	uint64_t address;
	uint32_t hwBpAddrLen;
	uint32_t hwBpAddrType;
	uint32_t hwBpThreadType;
	uint32_t hwBpKeepTimeMs;
};
#pragma pack()


#endif /* HWBPSERVER_H_ */