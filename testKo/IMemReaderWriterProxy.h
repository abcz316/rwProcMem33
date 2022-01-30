#ifndef MEM_READER_WRITER_PROXY_H_
#define MEM_READER_WRITER_PROXY_H_
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <mutex>
#include <thread>
#include <sstream>

#ifdef __linux__
#include <unistd.h>
#include <sys/sysinfo.h>
#else
#include <windows.h>
#endif


class IMemReaderWriterProxy
{
public:
	virtual BOOL ReadProcessMemory(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void *lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesRead,
		BOOL bIsForceRead = FALSE) = 0;
	virtual BOOL ReadProcessMemory_Fast(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void *lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesRead,
		BOOL bIsForceRead = FALSE)  = 0;
	virtual BOOL WriteProcessMemory(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void * lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesWritten,
		BOOL bIsForceWrite = FALSE) = 0;
	virtual BOOL WriteProcessMemory_Fast(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void * lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesWritten,
		BOOL bIsForceWrite = FALSE) = 0;
	virtual BOOL VirtualQueryExFull(
		uint64_t hProcess,
		BOOL showPhy,
		std::vector<DRIVER_REGION_INFO> & vOutput,
		BOOL & bOutListCompleted) = 0;
};

#endif /* MEM_READER_WRITER_PROXY_H_ */

