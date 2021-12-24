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
#endif

#include "../testKo/MemoryReaderWriter37.h"

class CMemReaderWriterProxy
{
public:
	CMemReaderWriterProxy() {}
	CMemReaderWriterProxy(CMemoryReaderWriter* pDriver) {
		m_spDriver = std::make_shared<CMemoryReaderWriter>();
		m_spDriver->SetLinkFD(pDriver->GetLinkFD());
	}
	CMemReaderWriterProxy(std::shared_ptr<CMemoryReaderWriter> spDriver) { m_spDriver = spDriver; }
	virtual BOOL ReadProcessMemory(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void *lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesRead,
		BOOL bIsForceRead = FALSE) {
		if (!m_spDriver) { return FALSE; }
		return m_spDriver->ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead, bIsForceRead);
	}
	virtual BOOL WriteProcessMemory(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void * lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesWritten,
		BOOL bIsForceWrite = FALSE) {
		if (!m_spDriver) { return FALSE; }
		return m_spDriver->WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten, bIsForceWrite);
	}
	virtual BOOL VirtualQueryExFull(
		uint64_t hProcess,
		BOOL showPhy,
		std::vector<DRIVER_REGION_INFO> & vOutput,
		BOOL & bOutListCompleted)
	{
		if (!m_spDriver) { return FALSE; }
		return m_spDriver->VirtualQueryExFull(hProcess, showPhy, vOutput, bOutListCompleted);
	}

private:
	std::shared_ptr<CMemoryReaderWriter> m_spDriver;
};

#endif /* MEM_READER_WRITER_PROXY_H_ */

