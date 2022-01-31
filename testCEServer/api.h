#pragma once
#include <stdint.h>
#include <pthread.h>
#include <sys/queue.h>
#include "porthelp.h"
#include "context.h"
#include "../testKo/MemoryReaderWriter37.h"
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


#define VQE_PAGEDONLY 1
#define VQE_DIRTYONLY 2
#define VQE_NOSHARED 4


struct ModuleListEntry {
	unsigned long long baseAddress;
	int moduleSize;
	std::string moduleName;

};

struct ProcessListEntry {
	int PID;
	std::string ProcessName;

};


#pragma pack(1)
struct RegionInfo {
	uint64_t baseaddress;
	uint64_t size;
	uint32_t protection;
	uint32_t type;
};
#pragma pack()


struct MyProcessInfo {
	int pid;
	size_t total_rss;
	std::string cmdline;
};

struct CeProcessList {
	std::vector<MyProcessInfo> vProcessList;
	decltype(vProcessList)::iterator readIter;
};

struct CeModuleList {
	std::vector<ModuleListEntry> vModuleList;
	decltype(vModuleList)::iterator readIter;
};

struct CeOpenProcess {
	int pid;
	uint64_t u64DriverProcessHandle;

	//短时间内保留上次获取的内存Maps，避免频繁调用驱动获取
	std::mutex mtxLockLastMaps; //访问冲突锁
	std::vector<DRIVER_REGION_INFO> vLastMaps;
	std::atomic<uint64_t> nLastGetMapsTime;
};

class CApi {
public:
	static BOOL InitReadWriteDriver(const char* lpszDevFileName);
	static HANDLE CreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID);
	static BOOL Process32First(HANDLE hSnapshot, ProcessListEntry & processentry);
	static BOOL Process32Next(HANDLE hSnapshot, ProcessListEntry &processentry);
	static BOOL Module32First(HANDLE hSnapshot, ModuleListEntry & moduleentry);
	static BOOL Module32Next(HANDLE hSnapshot, ModuleListEntry & moduleentry);
	static HANDLE OpenProcess(DWORD pid);
	static void CloseHandle(HANDLE h);
	static int VirtualQueryExFull(HANDLE hProcess, uint32_t flags, std::vector<RegionInfo> & vRinfo);
	static int VirtualQueryEx(HANDLE hProcess, uint64_t lpAddress, RegionInfo & rinfo, std::string & memName);
	static int ReadProcessMemory(HANDLE hProcess, void *lpAddress, void *buffer, int size);
	static int WriteProcessMemory(HANDLE hProcess, void *lpAddress, void *buffer, int size);
protected:
};
