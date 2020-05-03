#include "MemoryReaderWriter.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <malloc.h>

#define MAJOR_NUM 100
#define IOCTL_OPEN_PROCESS 								_IOR(MAJOR_NUM, 1, char*) //打开进程
#define IOCTL_CLOSE_HANDLE 								_IOR(MAJOR_NUM, 2, char*) //关闭进程
#define IOCTL_GET_PROCESS_MAPS_COUNT			_IOR(MAJOR_NUM, 3, char*) //获取进程的内存块地址数量
#define IOCTL_GET_PROCESS_MAPS_LIST				_IOR(MAJOR_NUM, 4, char*) //获取进程的内存块地址列表
#define IOCTL_CHECK_PROCESS_ADDR_PHY			_IOR(MAJOR_NUM, 5, char*) //检查进程的物理内存地址
#define IOCTL_GET_PROCESS_CMDLINE_ADDR		_IOR(MAJOR_NUM, 6, char*) //获取进程cmdline的内存地址
#define IOCTL_GET_PROCESS_RSS							_IOR(MAJOR_NUM, 7, char*) //获取进程的物理内存占用大小
# ifdef __cplusplus
extern "C" {
# endif

inline static int rwProcMemDriver_MyIoctl(int fd, unsigned int cmd, unsigned long buf, unsigned long bufSize)
{
	return ioctl(fd, cmd, buf);
}


int rwProcMemDriver_Connect(const char * lpszDevPath)
{
	pid_t pid = getpid();
	char filepath[256] = { 0 };
	sprintf(filepath, "/proc/%d/cmdline", pid);
	FILE *fp = fopen(filepath, "r");
	if (!fp)
	{
		printf("fopen /proc/%d/cmdline error():%s\n", pid, strerror(errno));
		return -100;
	}
	char filetext[4096] = { 0 };
	fgets(filetext, sizeof(filetext), fp);
	//printf("%s\n", filetext);
	fclose(fp);
	if (filetext[0] == '\x00')
	{
		printf("/proc/%d/cmdline empty\n", pid);
		return -101;
	}

	int nDriverLink = open(lpszDevPath, O_RDWR);
	if (nDriverLink <= 0)
	{
		int last_err = errno;
	
		printf("open error():%s\n", strerror(last_err));
		return -last_err;
	}


	uint64_t hProcess = rwProcMemDriver_OpenProcess(nDriverLink, pid);
	if (!hProcess)
	{
		printf("rwProcMemDriver_OpenProcess error():%s\n", strerror(errno));
		rwProcMemDriver_Disconnect(nDriverLink);
		return -102;
	}


	int init_cmdline_offset_ok = 0;
	for (int64_t relative_offset = -16; relative_offset <=16; relative_offset+=4)
	{
		char buf[16] = { 0 };
		memcpy(buf, &hProcess, 8);
		memcpy((void*)((size_t)buf + (size_t)8), &relative_offset, 8);
		int res = rwProcMemDriver_MyIoctl(nDriverLink, IOCTL_GET_PROCESS_CMDLINE_ADDR, &buf, sizeof(buf));
		//printf("ioctl %d\n", res);
		if (res == 0)
		{
			uint64_t arg_start;
			memcpy(&arg_start, &buf, 8);
			//printf("arg_start 0x%llx\n", arg_start);

			//uint64_t arg_end;
			//memcpy(&arg_end, (void*)((size_t)(&buf) + (size_t)8), 8);

			if (arg_start >= 0)
			{
				char temCmdLine[sizeof(filetext)] = { 0 };
				if (rwProcMemDriver_ReadProcessMemory(nDriverLink, hProcess, arg_start, temCmdLine, strlen(filetext) + 1, NULL))
				{
					//printf("temCmdLine %s\n", temCmdLine);
					if (strcmp(temCmdLine, filetext) == 0)
					{
						init_cmdline_offset_ok = 1;
						break;
					}
				}
			}
		}
	}

	rwProcMemDriver_CloseHandle(nDriverLink, hProcess);
	if (!init_cmdline_offset_ok)
	{
		printf("init cmdline offset failed\n");
		
		rwProcMemDriver_Disconnect(nDriverLink);
		return -103;
	}
	return nDriverLink;
}

BOOL rwProcMemDriver_Disconnect(int nDriverLink)
{
	if (nDriverLink < 0)
	{
		return FALSE;
	}
	//�Ͽ���������
	close(nDriverLink);
	return TRUE;
}

uint64_t rwProcMemDriver_OpenProcess(int nDriverLink, uint64_t pid)
{
	if (nDriverLink <= 0)
	{
		return 0;
	}
	char buf[8] = { 0 };
	memcpy(buf, &pid, 8);

	int res = rwProcMemDriver_MyIoctl(nDriverLink, IOCTL_OPEN_PROCESS, &buf,sizeof(buf));
	if (res != 0)
	{
		printf("OpenProcess ioctl():%s\n", strerror(errno));
		return 0;
	}
	uint64_t ptr = 0;
	memcpy(&ptr, &buf, 8);
	return ptr;
}


BOOL rwProcMemDriver_ReadProcessMemory(
	int nDriverLink,
	uint64_t hProcess,
	uint64_t lpBaseAddress,
	void * lpBuffer,
	size_t nSize,
	size_t * lpNumberOfBytesRead
)
{
	if(lpBaseAddress<=0)
	{
		return FALSE;
	}
	if (nDriverLink <= 0)
	{
		return FALSE;
	}
	if (!hProcess)
	{
		return FALSE;
	}
	if (nSize <= 0)
	{
		return FALSE;
	}
	int bufSize = nSize < 16 ? 16 : nSize;
	char *buf = (char*)malloc(bufSize);
	memset(buf, 0, bufSize);
	memcpy(buf, &hProcess, 8);
	memcpy((void*)((size_t)buf + (size_t)8), &lpBaseAddress, 8);
	ssize_t realRead = read(nDriverLink, buf, nSize);

	if (realRead < 0)
	{
		//printf("read(): %s\n", strerror(errno));
		free(buf);
		return FALSE;
	}
	if (realRead > 0)
	{
		memcpy(lpBuffer, buf, realRead);
	}

	if (lpNumberOfBytesRead)
	{
		*lpNumberOfBytesRead = realRead;
	}
	free(buf);
	return TRUE;

}
BOOL rwProcMemDriver_WriteProcessMemory(
	int nDriverLink,
	uint64_t hProcess,
	uint64_t lpBaseAddress,
	void * lpBuffer,
	size_t nSize,
	size_t * lpNumberOfBytesWritten)
{
	if(lpBaseAddress<=0)
	{
		return FALSE;
	}
	if (nDriverLink <= 0)
	{
		return FALSE;
	}
	if (!hProcess)
	{
		return FALSE;
	}
	if (nSize <= 0)
	{
		return FALSE;
	}
	
	int bufSize = nSize + 16;
	char *buf = (char*)malloc(bufSize);
	memset(buf, 0, bufSize);
	memcpy(buf, &hProcess, 8);
	memcpy((void*)((size_t)buf + (size_t)8), &lpBaseAddress, 8);
	memcpy((void*)((size_t)buf + (size_t)16), lpBuffer, nSize);

	ssize_t realWrite = write(nDriverLink, buf, nSize);
	if (realWrite < 0)
	{
		//printf("write(): %s\n", strerror(errno));
		free(buf);
		return FALSE;
	}

	if (lpNumberOfBytesWritten)
	{
		*lpNumberOfBytesWritten = realWrite;
	}
	free(buf);
	return TRUE;

}


BOOL rwProcMemDriver_CloseHandle(int nDriverLink, uint64_t handle)
{
	if (nDriverLink <= 0)
	{
		return FALSE;
	}
	if (!handle)
	{
		return FALSE;
	}

	char buf[8] = { 0 };
	memcpy(buf, &handle, 8);

	int res = rwProcMemDriver_MyIoctl(nDriverLink, IOCTL_CLOSE_HANDLE, &buf, sizeof(buf));
	if (res != 0)
	{
		printf("CloseHandle ioctl():%s\n", strerror(errno));
		return FALSE;
	}
	return TRUE;
}

BOOL rwProcMemDriver_VirtualQueryExFull(int nDriverLink, uint64_t hProcess, BOOL showPhy, cvector vOutput)
{
	if (nDriverLink <= 0)
	{
		return FALSE;
	}
	if (!hProcess)
	{
		return FALSE;
	}
	char buf[8] = { 0 };
	memcpy(buf, &hProcess, 8);
	int count = rwProcMemDriver_MyIoctl(nDriverLink, IOCTL_GET_PROCESS_MAPS_COUNT, &buf, sizeof(buf));
	//printf("count %d\n", count);
	if (count <= 0)
	{
		//printf("ioctl():%s\n", strerror(errno));
		return FALSE;
	}

	uint64_t big_buf_len = (1+8 + 8 + 4 + 4096) * (count + 20);
	char * big_buf = (char*)malloc(big_buf_len);
	memset(big_buf, 0, big_buf_len);
	memcpy(big_buf, &hProcess, 8);

	uint64_t name_len = 4096;
	memcpy((void*)((size_t)big_buf + (size_t)8), &name_len, 8);
	memcpy((void*)((size_t)big_buf + (size_t)16), &big_buf_len, 8);

	int res = rwProcMemDriver_MyIoctl(nDriverLink, IOCTL_GET_PROCESS_MAPS_LIST, big_buf, big_buf_len);
	//printf("res %d\n", res);
	if (res <= 0)
	{
		//printf("ioctl():%s\n", strerror(errno));
		free(big_buf);
		return FALSE;
	}
	size_t copy_pos = (size_t)big_buf;
	unsigned char pass = *(unsigned char*)big_buf;
	if (pass == '\x01')
	{
		//printf("pass %x\n", pass);
		free(big_buf);
		return FALSE;
	}
	copy_pos++;
	for (; res>0 ; res--)
	{
		uint64_t vma_start = 0;
		uint64_t vma_end = 0;
		char vma_flags[4] = { 0 };
		char name[4096] = { 0 };
		memcpy(&vma_start, (void*)copy_pos, 8);
		copy_pos += 8;
		memcpy(&vma_end, (void*)copy_pos, 8);
		copy_pos += 8;
		memcpy(&vma_flags, (void*)copy_pos, 4);
		copy_pos += 4;
		memcpy(&name, (void*)copy_pos, 4096);
		copy_pos += 4096;

		Driver_RegionInfo rInfo = { 0 };
		rInfo.baseaddress = vma_start;
		rInfo.size = vma_end - vma_start;
		if (vma_flags[2] == '\x01')
		{
			//executable
			if (vma_flags[1] == '\x01')
			{
				rInfo.protection = PAGE_EXECUTE_READWRITE;
			}
			else
			{
				rInfo.protection = PAGE_EXECUTE_READ;
			}
		}
		else
		{
			//not executable
			if (vma_flags[1] == '\x01')
			{
				rInfo.protection = PAGE_READWRITE;
			}
			else if (vma_flags[0] == '\x01')
			{
				rInfo.protection = PAGE_READONLY;
			}
			else
			{
				rInfo.protection = PAGE_NOACCESS;
			}
		}
		if (vma_flags[3] == '\x01')
		{
			rInfo.type = MEM_MAPPED;
		}
		else
		{
			rInfo.type = MEM_PRIVATE;
		}
		memcpy(&rInfo.name, &name, 4096);

		if (showPhy)
		{
			//只显示在物理内存中的内存
			Driver_RegionInfo rPhyInfo = { 0 };

			char ptr_buf[16] = { 0 };
			memcpy(ptr_buf, &hProcess, 8);

			uint64_t addr;
			int isPhyRegion = 0;
			for (addr = vma_start; addr < vma_end; addr += getpagesize())
			{
				memcpy((void*)((size_t)ptr_buf + (size_t)8), &addr, 8);
				if (rwProcMemDriver_MyIoctl(nDriverLink, IOCTL_CHECK_PROCESS_ADDR_PHY, &ptr_buf, sizeof(ptr_buf)) == 1)
				{
					if (isPhyRegion == 0)
					{
						isPhyRegion = 1;
						rPhyInfo.baseaddress = addr;
						rPhyInfo.protection = rInfo.protection;
						rPhyInfo.type = rInfo.type;
						strcpy(rPhyInfo.name, rInfo.name);
					}

				}
				else
				{
					if (isPhyRegion == 1)
					{
						isPhyRegion = 0;
						rPhyInfo.size = addr - rPhyInfo.baseaddress;
						cvector_pushback(vOutput, &rPhyInfo);
					}
				}
			}

			if (isPhyRegion == 1)
			{
				//all vma region inside phy memory
				rPhyInfo.size = vma_end - rPhyInfo.baseaddress;
				cvector_pushback(vOutput, &rPhyInfo);
			}

		}
		else
		{
			//显示全部内存
			cvector_pushback(vOutput, &rInfo);
		}

	}
	free(big_buf);
	return TRUE;
}


BOOL rwProcMemDriver_GetProcessRSS(int nDriverLink, uint64_t hProcess, uint64_t *outRss)
{
	if (nDriverLink <= 0)
	{
		return FALSE;
	}
	if (!hProcess)
	{
		return FALSE;
	}
	char buf[8] = { 0 };
	memcpy(buf, &hProcess, 8);
	int res = rwProcMemDriver_MyIoctl(nDriverLink, IOCTL_GET_PROCESS_RSS, &buf, sizeof(buf));
	if (res != 0)
	{
		//printf("ioctl():%s\n", strerror(errno));
		return FALSE;
	}
	memcpy(outRss, &buf, 8);
	return TRUE;
}
BOOL rwProcMemDriver_GetProcessCmdline(int nDriverLink, uint64_t hProcess, char *outCmdlineBuf, size_t bufSize)
{
	if (nDriverLink <= 0)
	{
		return FALSE;
	}
	if (!hProcess)
	{
		return FALSE;
	}
	if (bufSize <= 0)
	{
		return FALSE;
	}
	char buf[16] = { 0 };
	memcpy(buf, &hProcess, 8);
	uint64_t relative_offset = -1;
	memcpy((void*)((size_t)buf + (size_t)8), &relative_offset, 8);
	int res = rwProcMemDriver_MyIoctl(nDriverLink, IOCTL_GET_PROCESS_CMDLINE_ADDR, &buf, sizeof(buf));
	if (res != 0)
	{
		//printf("ioctl():%s\n", strerror(errno));
		return FALSE;
	}
	uint64_t arg_start, arg_end;
	memcpy(&arg_start, &buf, 8);
	memcpy(&arg_end, (void*)((size_t)(&buf)+ (size_t)8), 8);
	if (arg_start <= 0)
	{
		return FALSE;
	}
	uint64_t len = arg_end - arg_start;
	if (len<= 0)
	{
		return FALSE;
	}
	if (bufSize< len)
	{
		len = bufSize;
	}
	memset(outCmdlineBuf, 0, bufSize);
	return rwProcMemDriver_ReadProcessMemory(nDriverLink, hProcess, arg_start, outCmdlineBuf, len, NULL);
}

# ifdef __cplusplus
}
# endif
