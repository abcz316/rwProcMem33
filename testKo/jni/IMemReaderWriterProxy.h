#ifndef MEM_READER_WRITER_PROXY_H_
#define MEM_READER_WRITER_PROXY_H_
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <mutex>
#include <thread>
#include <sstream>
#include <stdint.h>

#ifdef __linux__
#include <unistd.h>
#include <sys/sysinfo.h>
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define PAGE_NOACCESS 1
#define PAGE_READONLY 2
#define PAGE_READWRITE 4
#define PAGE_WRITECOPY 8
#define PAGE_EXECUTE 16
#define PAGE_EXECUTE_READ 32
#define PAGE_EXECUTE_READWRITE 64

#define MEM_MAPPED 262144
#define MEM_PRIVATE 131072
#else
#include <windows.h>
#endif

#pragma pack(1)
typedef struct {
	uint64_t baseaddress;
	uint64_t size;
	uint32_t protection;
	uint32_t type;
	char name[4096];
} DRIVER_REGION_INFO, *PDRIVER_REGION_INFO;
#pragma pack()


struct IMemReaderWriterProxy {
	virtual BOOL ReadProcessMemory(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void *lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesRead = NULL,
		BOOL bIsForceRead = FALSE) = 0;
	virtual BOOL ReadProcessMemory_Fast(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void *lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesRead = NULL,
		BOOL bIsForceRead = FALSE) = 0;
	virtual BOOL WriteProcessMemory(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void * lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesWritten = NULL,
		BOOL bIsForceWrite = FALSE) = 0;
	virtual BOOL WriteProcessMemory_Fast(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void * lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesWritten = NULL,
		BOOL bIsForceWrite = FALSE) = 0;
	virtual BOOL VirtualQueryExFull(
		uint64_t hProcess,
		BOOL showPhy,
		std::vector<DRIVER_REGION_INFO> & vOutput,
		BOOL & bOutListCompleted) = 0;

	virtual BOOL CheckMemAddrIsValid(
		uint64_t hProcess,
		uint64_t lpBaseAddress) = 0;
};

#endif /* MEM_READER_WRITER_PROXY_H_ */

