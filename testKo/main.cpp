#include <cstdio>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <memory>
#include <vector>
#include <dirent.h>

#include "MemoryReaderWriter.h"

//默认驱动文件名
#define DEV_FILENAME "/dev/rwProcMem33"



#pragma pack(1)
struct MyProcessInfo {
	int pid;
	size_t total_rss;
	char cmdline[4096];
};
#pragma pack()


//获取进程列表信息
BOOL GetProcessListInfo(CMemoryReaderWriter *pDriver, bool bGetPhyMemorySize, std::vector<MyProcessInfo> & vOutput)
{
	DIR *dir = NULL;
	struct dirent *ptr = NULL;

	dir = opendir("/proc");
	if (NULL != dir)
	{
		while ((ptr = readdir(dir)) != NULL)   // 循环读取路径下的每一个文件/文件夹
		{
			// 如果读取到的是"."或者".."则跳过，读取到的不是文件夹名字也跳过
			if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0))
			{
				continue;
			}
			else if (ptr->d_type != DT_DIR)
			{
				continue;
			}
			else if (strspn(ptr->d_name, "1234567890") != strlen(ptr->d_name))
			{
				continue;
			}

			int pid = atoi(ptr->d_name);
			uint64_t hProcess = pDriver->OpenProcess(pid);
			if (!hProcess)
			{
				continue;
			}

			MyProcessInfo pInfo = { 0 };
			pInfo.pid = pid;

			if (bGetPhyMemorySize)
			{
				uint64_t outRss = 0;
				pDriver->GetProcessRSS(hProcess, &outRss);
				pInfo.total_rss = outRss;
			}
		

			
			char cmdline[100] = { 0 };
			pDriver->GetProcessCmdline(hProcess, cmdline, sizeof(cmdline));
			strcpy(pInfo.cmdline, cmdline);

			pDriver->CloseHandle(hProcess);

			vOutput.push_back(pInfo);
		}
		closedir(dir);
		return TRUE;
	}
	return FALSE;
}

int main(int argc, char *argv[])
{
	printf(
	"======================================================\n"
	"本驱动名称: Linux  ARM64 硬件读写进程内存驱动33\n"
	"本驱动接口列表：\n"
		"\t1.  驱动_打开进程: OpenProcess\n"
		"\t2.  驱动_读取进程内存: ReadProcessMemory\n"
		"\t3.  驱动_写入进程内存: WriteProcessMemory\n"
		"\t4.  驱动_关闭进程: CloseHandle\n"
		"\t5.  驱动_获取进程内存块列表: VirtualQueryExFull（可选：显示全部内存、只显示在物理内存中的内存）\n"
		"\t6.  驱动_获取进程占用物理内存大小: GetProcessRSS\n"
		"\t7.  驱动_获取进程命令行: GetProcessCmdline\n"
		"\t以上所有功能不注入、不附加进程，不打开进程任何文件，所有操作均为硬件操作\n"
	"======================================================\n"
	);

	pid_t pid = getpid();
	printf("当前进程PID :%d\n", pid);

	char szBuf[1024];
	memset(szBuf, 0, sizeof(szBuf));
	snprintf(szBuf, sizeof(szBuf), "%s", "文本123");

	void *pBuf = &szBuf;
	printf("当前缓冲区内容:%s,当前缓冲区的内存地址:%p\n", szBuf, pBuf);


	printf("===========开始驱动操作演示===========================\n");

	CMemoryReaderWriter driver;

	//驱动默认文件名
	std::string devFileName = DEV_FILENAME;
	if (argc > 1)
	{
		//如果用户自定义输入驱动名
		devFileName = argv[1];
	}
	printf("Connecting driver:%s\n", devFileName.c_str());


	//连接驱动
	int err = 0;
	if (!driver.ConnectDriver(DEV_FILENAME, err))
	{
		printf("Connect driver failed. error:%d\n", err);
		fflush(stdout);
		return 0;
	}
	
	//驱动_打开进程
	uint64_t hProcess = driver.OpenProcess(pid);
	printf("调用驱动 OpenProcess 返回值:%lld\n", hProcess);
	if (!hProcess)
	{
		printf("调用驱动 OpenProcess 失败\n");
		fflush(stdout);
		return 0;
	}



	//驱动_读取进程内存
	char readBuf[1024] = { 0 };
	size_t real_read = 0;
	auto read_res = driver.ReadProcessMemory(hProcess, (uint64_t)pBuf, &readBuf, sizeof(readBuf), &real_read);
	printf("调用驱动 ReadProcessMemory 读取内存地址:%p,返回值:%d,读取到的内容:%s,实际读取大小:%d\n", pBuf, read_res, readBuf, real_read);

	//驱动_写入进程内存
	memset(readBuf, 0, sizeof(readBuf));
	snprintf(readBuf, sizeof(readBuf), "%s", "写入456");
	size_t real_write = 0;
	auto write_res = driver.WriteProcessMemory(hProcess, (uint64_t)pBuf, &readBuf, sizeof(readBuf), &real_write);
	printf("调用驱动 WriteProcessMemory 写入内存地址:%p,返回值:%d,写入的内容:%s,实际写入大小:%d\n", pBuf, write_res, readBuf, real_write);

	printf("当前缓冲区内容 :%s,当前缓冲区的内存地址:%p\n", szBuf, pBuf);

	

	//驱动_获取进程内存块列表（显示全部内存）
	std::vector<Driver_RegionInfo> vMaps;
	BOOL b = driver.VirtualQueryExFull(hProcess, FALSE, vMaps);
	printf("调用驱动 VirtualQueryExFull(显示全部内存) 返回值:%d\n", b);

	//显示进程内存块地址列表
	for (Driver_RegionInfo rinfo : vMaps)
	{
		printf("---Start:%llx,Size:%lld,Protection:%d,Type:%d,Name:%s\n", rinfo.baseaddress, rinfo.size, rinfo.protection, rinfo.type, rinfo.name);
	}
	

	//驱动_获取进程内存块列表（只显示在物理内存中的内存）
	vMaps.clear();
	b = driver.VirtualQueryExFull(hProcess, TRUE, vMaps);
	printf("调用驱动 VirtualQueryExFull(只显示在物理内存中的内存) 返回值:%d\n", b);

	//显示进程内存块地址列表
	for (Driver_RegionInfo rinfo : vMaps)
	{
		printf("+++Start:%llx,Size:%lld,Protection:%d,Type:%d,Name:%s\n", rinfo.baseaddress, rinfo.size, rinfo.protection, rinfo.type, rinfo.name);
	}


	//驱动_获取进程占用物理内存大小
	uint64_t outRss = 0;
	b = driver.GetProcessRSS(hProcess, &outRss);
	printf("调用驱动 GetProcessRSS 返回值:%d,当前进程占用物理内存大小:%ldKB\n", b, outRss);
	

	//驱动_获取进程命令行
	char cmdline[100] = { 0 };
	b = driver.GetProcessCmdline(hProcess, cmdline, sizeof(cmdline));
	printf("调用驱动 GetProcessCmdline 返回值:%d,当前进程命令行:%s\n", b, cmdline);

	//驱动_关闭进程
	driver.CloseHandle(hProcess);
	printf("调用驱动 CloseHandle %lld\n", hProcess);

	//打印进程列表信息
	std::vector<MyProcessInfo> vProcessList;
	GetProcessListInfo(&driver, false, vProcessList);
	//显示进程列表
	for (MyProcessInfo pinfo : vProcessList)
	{
		printf("pid:%d,rss:%d,cmdline:%s\n", pinfo.pid, pinfo.total_rss, pinfo.cmdline);
	}
	fflush(stdout);
	return 0;
}
