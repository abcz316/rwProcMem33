#ifndef MEMORY_READER_WRITER_H_
#define MEMORY_READER_WRITER_H_

#include <stdio.h>
#include <stdint.h>
#include "cvector.h"

#ifndef __cplusplus
#define true 1
#define false 0
typedef int bool;
#endif

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


#pragma pack(1)
typedef struct {
	uint64_t baseaddress;
	uint64_t size;
	uint32_t protection;
	uint32_t type;
	char name[4096];
} Driver_RegionInfo, *PDriver_RegionInfo;
#pragma pack()


# ifdef __cplusplus
extern "C" {
# endif
int rwProcMemDriver_Connect(const char* lpszDevPath);
BOOL rwProcMemDriver_Disconnect(int nDriverLink);
uint64_t rwProcMemDriver_OpenProcess(int nDriverLink, uint64_t pid);
BOOL rwProcMemDriver_ReadProcessMemory(
	int nDriverLink,
	uint64_t hProcess,
	uint64_t lpBaseAddress,
	void * lpBuffer,
	size_t nSize,
	size_t * lpNumberOfBytesRead
);
BOOL rwProcMemDriver_WriteProcessMemory(
	int nDriverLink,
	uint64_t hProcess,
	uint64_t lpBaseAddress,
	void * lpBuffer,
	size_t nSize,
	size_t * lpNumberOfBytesWritten);
BOOL rwProcMemDriver_CloseHandle(int nDriverLink, uint64_t handle);
BOOL rwProcMemDriver_VirtualQueryExFull(int nDriverLink, uint64_t hProcess, BOOL showPhy, cvector vOutput);
BOOL rwProcMemDriver_GetProcessRSS(int nDriverLink, uint64_t hProcess, uint64_t *outRss);
BOOL rwProcMemDriver_GetProcessCmdline(int nDriverLink, uint64_t hProcess, char *outCmdlineBuf, size_t bufSize);
# ifdef __cplusplus
}
# endif


#ifdef __cplusplus
#include <vector>
#include <mutex>
class CMemoryReaderWriter
{
public:

	CMemoryReaderWriter()
	{

	}
	~CMemoryReaderWriter()
	{
		DisconnectDriver();
	}


	//连接驱动（驱动/dev路径名，错误代码）
	BOOL ConnectDriver(const char* lpszDevPath, int & err)
	{
		if (m_nDriverLink > 0)
		{
			return TRUE;
		}
		m_nDriverLink = rwProcMemDriver_Connect(lpszDevPath);
		if (m_nDriverLink <= 0)
		{
			err = m_nDriverLink;
			return FALSE;
		}
		else
		{
			err = 0;
		}
		return TRUE;
	}

	//断开驱动
	BOOL DisconnectDriver()
	{
		if (m_nDriverLink > 0)
		{
			rwProcMemDriver_Disconnect(m_nDriverLink);
			m_nDriverLink = -1;
			return TRUE;
		}
		return FALSE;
	}

	//驱动是否连接正常
	BOOL IsDriverConnected()
	{
		return m_nDriverLink > 0 ? TRUE : FALSE;
	}
	

	//驱动_打开进程
	uint64_t OpenProcess(uint64_t pid)
	{
		return rwProcMemDriver_OpenProcess(m_nDriverLink, pid);
	}


	//驱动_读取进程内存
	BOOL ReadProcessMemory(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void *lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesRead)
	{
		return rwProcMemDriver_ReadProcessMemory(
				m_nDriverLink,
				hProcess,
				lpBaseAddress,
				lpBuffer,
				nSize,
				lpNumberOfBytesRead);
	}
	
	//驱动_写入进程内存
	BOOL WriteProcessMemory(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void * lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesWritten)
	{
		return rwProcMemDriver_WriteProcessMemory(
			m_nDriverLink,
			hProcess,
			lpBaseAddress,
			lpBuffer,
			nSize,
			lpNumberOfBytesWritten);
	}
	//驱动_关闭进程
	BOOL CloseHandle(uint64_t handle)
	{
		return rwProcMemDriver_CloseHandle(m_nDriverLink, handle);
	}

	//驱动_获取进程内存块列表
	//（参数showPhy说明: FALSE为显示全部内存，TRUE为只显示在物理内存中的内存，注意：如果进程内存不存在于物理内存中，驱动将无法读取）
	BOOL VirtualQueryExFull(uint64_t hProcess, BOOL showPhy,  std::vector<Driver_RegionInfo> & vOutput)
	{
		cvector cvOutput = cvector_create(sizeof(Driver_RegionInfo));
		BOOL b = rwProcMemDriver_VirtualQueryExFull(m_nDriverLink, hProcess, showPhy, cvOutput);
		for (citerator iter = cvector_begin(cvOutput); iter != cvector_end(cvOutput); iter = cvector_next(cvOutput, iter))
		{
			Driver_RegionInfo *rinfo = (Driver_RegionInfo*)iter;
			vOutput.push_back(*rinfo);
		}
		cvector_destroy(cvOutput);
		return b;
	}

	//驱动_获取进程占用物理内存大小
	BOOL GetProcessRSS(uint64_t hProcess, uint64_t * outRss)
	{
		return rwProcMemDriver_GetProcessRSS(m_nDriverLink, hProcess, outRss);
	}

	//驱动_获取进程命令行
	BOOL GetProcessCmdline(uint64_t hProcess, char *outCmdlineBuf, size_t bufSize)
	{
		return rwProcMemDriver_GetProcessCmdline(m_nDriverLink, hProcess, outCmdlineBuf, bufSize);
	}

private:
	int m_nDriverLink = -1;
};
#endif





#endif /* MEMORY_READER_WRITER_H_ */
