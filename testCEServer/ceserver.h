#ifndef CESERVER_H_
#define CESERVER_H_

#include <stdint.h>
#include <sys/types.h>
#include "porthelp.h"

#define CMD_GETVERSION 0
#define CMD_CLOSECONNECTION 1
#define CMD_TERMINATESERVER 2
#define CMD_OPENPROCESS 3
#define CMD_CREATETOOLHELP32SNAPSHOT 4
#define CMD_PROCESS32FIRST 5
#define CMD_PROCESS32NEXT 6
#define CMD_CLOSEHANDLE 7
#define CMD_VIRTUALQUERYEX 8
#define CMD_READPROCESSMEMORY 9
#define CMD_WRITEPROCESSMEMORY 10
#define CMD_STARTDEBUG 11
#define CMD_STOPDEBUG 12
#define CMD_WAITFORDEBUGEVENT 13
#define CMD_CONTINUEFROMDEBUGEVENT 14
#define CMD_SETBREAKPOINT 15
#define CMD_REMOVEBREAKPOINT 16
#define CMD_SUSPENDTHREAD 17
#define CMD_RESUMETHREAD 18
#define CMD_GETTHREADCONTEXT 19
#define CMD_SETTHREADCONTEXT 20
#define CMD_GETARCHITECTURE 21
#define CMD_MODULE32FIRST 22
#define CMD_MODULE32NEXT 23

#define CMD_GETSYMBOLLISTFROMFILE 24
#define CMD_LOADEXTENSION         25

#define CMD_ALLOC                   26
#define CMD_FREE                    27
#define CMD_CREATETHREAD            28
#define CMD_LOADMODULE              29
#define CMD_SPEEDHACK_SETSPEED      30

#define CMD_VIRTUALQUERYEXFULL      31
#define CMD_GETREGIONINFO           32

#define CMD_AOBSCAN					200

//just in case I ever get over 255 commands this value will be reserved for a secondary command list (FF 00 -  FF 01 - ... - FF FE - FF FF 01 - FF FF 02 - .....
#define CMD_COMMANDLIST2            255





//extern char *versionstring;

#pragma pack(1)
struct CeVersion {
	int version;
	unsigned char stringsize;
	//append the versionstring
};

struct CeCreateToolhelp32Snapshot {
	DWORD dwFlags;
	DWORD th32ProcessID;
};

struct CeProcessEntry {
	int result;
	int pid;
	int processnamesize;
	//processname
};

struct CeModuleEntry {
	int result;
	int64_t modulebase;
	int modulesize;
	int modulenamesize;
	//modulename

};

struct CeVirtualQueryExInput {
	int handle;
	uint64_t baseaddress;
};

struct CeVirtualQueryExOutput {
	uint8_t result;
	uint32_t protection;
	uint32_t type;
	uint64_t baseaddress;
	uint64_t size;
};

struct CeVirtualQueryExFullInput {
	int handle;
	uint8_t flags;
};

struct CeVirtualQueryExFullOutput {
	uint32_t protection;
	uint32_t type;
	uint64_t baseaddress;
	uint64_t size;
};

struct CeReadProcessMemoryInput {
	uint32_t handle;
	uint64_t address;
	uint32_t size;
	uint8_t  compress;
};

struct CeReadProcessMemoryOutput {
	int read;
};

struct CeWriteProcessMemoryInput {
	int32_t handle;
	int64_t address;
	int32_t size;
};


struct CeWriteProcessMemoryOutput {
	int32_t written;
};


struct CeSetBreapointInput {
	HANDLE hProcess;
	int tid;
	int debugreg;
	uint64_t Address;
	int bptype;
	int bpsize;
};


struct CeSetBreapointOutput {
	int result;
};

struct CeRemoveBreapointInput {
	HANDLE hProcess;
	uint32_t tid;
	uint32_t debugreg;
	uint32_t wasWatchpoint;
};


struct CeRemoveBreapointOutput {
	int result;
};

struct CeSuspendThreadInput {
	HANDLE hProcess;
	int tid;
};


struct CeSuspendThreadOutput {
	int result;
};

struct CeResumeThreadInput {
	HANDLE hProcess;
	int tid;
};


struct CeResumeThreadOutput {
	int result;
};

struct CeAllocInput {
	HANDLE hProcess;
	uint64_t preferedBase;
	uint32_t size;
};


struct CeAllocOutput {
	uint64_t address; //0=fail
};

struct CeFreeInput {
	HANDLE hProcess;
	uint64_t address;
	uint32_t size;
};


struct CeFreeOutput {
	uint32_t result;
};

struct CeCreateThreadInput {
	HANDLE hProcess;
	uint64_t startaddress;
	uint64_t parameter;
};


struct CeCreateThreadOutput {
	HANDLE threadhandle;
};

struct CeLoadModuleInput {
	HANDLE hProcess;
	uint32_t modulepathlength;
	//modulepath
};


struct CeLoadModuleOutput {
	uint32_t result;
};


struct CeSpeedhackSetSpeedInput {
	HANDLE hProcess;
	float speed;
};


struct CeSpeedhackSetSpeedOutput {
	uint32_t result;
};

struct CeAobScanInput {
	HANDLE hProcess;
	uint64_t start;
	uint64_t end;
	int inc;
	int protection;
	int scansize;
};
#pragma pack()


#endif /* CESERVER_H_ */