#include <cstdio>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <memory>
#include <vector>
#include <dirent.h>
#include <cinttypes>
#include "MemoryReaderWriter37.h"
#include "../testMemSearch/MapRegionType.h"


int main(int argc, char *argv[]) {
	printf(
		"======================================================\n"
		"本驱动名称: Linux ARM64 硬件读写进程内存驱动37\n"
		"本驱动接口列表：\n"
		"\t1.	 驱动_设置驱动设备接口文件允许同时被使用的最大值: SetMaxDevFileOpen\n"
		"\t2.	 驱动_隐藏驱动（卸载驱动需重启机器）: HideKernelModule\n"
		"\t3.	 驱动_打开进程: OpenProcess\n"
		"\t4.	 驱动_读取进程内存: ReadProcessMemory\n"
		"\t5.	 驱动_写入进程内存: WriteProcessMemory\n"
		"\t6.	 驱动_关闭进程: CloseHandle\n"
		"\t7.	 驱动_获取进程内存块列表: VirtualQueryExFull（可选：显示全部内存、只显示在物理内存中的内存）\n"
		"\t8.	 驱动_获取进程PID列表: GetProcessPidList\n"
		"\t9.	 驱动_获取进程权限等级: GetProcessGroup\n"
		"\t10.驱动_提升进程权限到Root: SetProcessRoot\n"
		"\t11.驱动_获取进程占用物理内存大小: GetProcessRSS\n"
		"\t12.驱动_获取进程命令行: GetProcessCmdline\n"
		"\t以上所有功能不注入、不附加进程，不打开进程任何文件，所有操作均为内核操作\n"
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

	CMemoryReaderWriter rwDriver;

	//驱动默认文件名
	std::string devFileName = RWPROCMEM_FILE_NODE;
	if (argc > 1) {
		//如果用户自定义输入驱动名
		devFileName = argv[1];
	}
	printf("Connecting rwDriver:%s\n", devFileName.c_str());


	//连接驱动
	int err = 0;
	if (!rwDriver.ConnectDriver(devFileName.c_str(), err)) {
		printf("Connect rwDriver failed. error:%d\n", err);
		fflush(stdout);
		return 0;
	}

	//驱动_设置驱动设备接口文件允许同时被使用的最大值
	BOOL b = rwDriver.SetMaxDevFileOpen(1);
	printf("调用驱动 SetMaxDevFileOpen 返回值:%d\n", b);


	/*
	//驱动_隐藏驱动（卸载驱动需重启机器）
	b = rwDriver.HideKernelModule();
	printf("调用驱动 HideKernelModule 返回值:%d\n", b);
	*/

	//驱动_打开进程
	uint64_t hProcess = rwDriver.OpenProcess(pid);
	printf("调用驱动 OpenProcess 返回值:%" PRIu64 "\n", hProcess);
	if (!hProcess) {
		printf("调用驱动 OpenProcess 失败\n");
		fflush(stdout);
		return 0;
	}


	//驱动_读取进程内存
	char readBuf[1024] = { 0 };
	size_t real_read = 0;
	//如果是单线程读内存，还可另选用极速版函数：ReadProcessMemory_Fast
	BOOL read_res = rwDriver.ReadProcessMemory(hProcess, (uint64_t)pBuf, &readBuf, sizeof(readBuf), &real_read, FALSE);
	printf("调用驱动 ReadProcessMemory 读取内存地址:%p,返回值:%d,读取到的内容:%s,实际读取大小:%zu\n", pBuf, read_res, readBuf, real_read);

	//驱动_写入进程内存
	memset(readBuf, 0, sizeof(readBuf));
	snprintf(readBuf, sizeof(readBuf), "%s", "写入456");
	size_t real_write = 0;
	//如果是单线程写内存，还可另选用极速版函数：WriteProcessMemory_Fast
	BOOL write_res = rwDriver.WriteProcessMemory(hProcess, (uint64_t)pBuf, &readBuf, sizeof(readBuf), &real_write, FALSE);
	printf("调用驱动 WriteProcessMemory 写入内存地址:%p,返回值:%d,写入的内容:%s,实际写入大小:%zu\n", pBuf, write_res, readBuf, real_write);

	printf("当前缓冲区内容 :%s,当前缓冲区的内存地址:%p\n", szBuf, pBuf);


	//驱动_获取进程内存块列表（显示全部内存）
	std::vector<DRIVER_REGION_INFO> vMaps;
	BOOL bOutListCompleted;
	b = rwDriver.VirtualQueryExFull(hProcess, FALSE, vMaps, bOutListCompleted);
	printf("调用驱动 VirtualQueryExFull(显示全部内存) 返回值:%d\n", b);

	//显示进程内存块地址列表
	for (DRIVER_REGION_INFO rinfo : vMaps) {
		printf("---Start:%p,Size:%" PRIu64 ",Type:%s,Name:%s\n", (void*)rinfo.baseaddress, rinfo.size, MapsTypeToString(&rinfo).c_str(), rinfo.name);
	}


	//驱动_获取进程内存块列表（只显示在物理内存中的内存）
	vMaps.clear();
	b = rwDriver.VirtualQueryExFull(hProcess, TRUE, vMaps, bOutListCompleted);
	printf("调用驱动 VirtualQueryExFull(只显示在物理内存中的内存) 返回值:%d\n", b);

	//显示进程内存块地址列表
	for (DRIVER_REGION_INFO rinfo : vMaps) {
		printf("+++Start:%p,Size:%" PRIu64 ",Type:%s,Name:%s\n", (void*)rinfo.baseaddress, rinfo.size, MapsTypeToString(&rinfo).c_str(), rinfo.name);
	}



	//驱动_获取进程占用物理内存大小
	uint64_t outRss = 0;
	b = rwDriver.GetProcessRSS(hProcess, outRss);
	printf("调用驱动 GetProcessRSS 返回值:%d,当前进程占用物理内存大小:%" PRIu64 "KB\n", b, outRss);


	//驱动_获取进程命令行
	char cmdline[100] = { 0 };
	b = rwDriver.GetProcessCmdline(hProcess, cmdline, sizeof(cmdline));
	printf("调用驱动 GetProcessCmdline 返回值:%d,当前进程命令行:%s\n", b, cmdline);


	//驱动_获取进程权限等级
	uint64_t  nOutUID, nOutSUID, nOutEUID, nOutFSUID, nOutGID, nOutSGID, nOutEGID, nOutFSGID;
	b = rwDriver.GetProcessGroup(hProcess, nOutUID, nOutSUID, nOutEUID, nOutFSUID, nOutGID, nOutSGID, nOutEGID, nOutFSGID);
	printf("调用驱动 GetProcessGroup 返回值:%d,"
		"nOutUID:%" PRIu64 ", nOutSUID:%" PRIu64 ", nOutEUID:%" PRIu64 ","
		"nOutFSUID:%" PRIu64 ", nOutGID:%" PRIu64 ", nOutSGID:%" PRIu64 ","
		"nOutEGID:%" PRIu64 ", nOutFSGID:%" PRIu64 "\n",
		b, nOutUID, nOutSUID, nOutEUID, nOutFSUID, nOutGID, nOutSGID, nOutEGID, nOutFSGID);


	//驱动_关闭进程
	rwDriver.CloseHandle(hProcess);
	printf("调用驱动 CloseHandle %" PRIu64 "\n", hProcess);


	//驱动_获取进程PID列表
	std::vector<int> vPID;
	b = rwDriver.GetProcessPidList(vPID, FALSE, bOutListCompleted);
	printf("调用驱动 GetProcessPidList 返回值:%d\n", b);

	//打印进程列表信息
	for (int pid : vPID) {
		//驱动_打开进程
		uint64_t hProcess = rwDriver.OpenProcess(pid);
		if (!hProcess) { continue; }


		//驱动_获取进程占用物理内存大小
		uint64_t outRss = 0;
		rwDriver.GetProcessRSS(hProcess, outRss);

		//驱动_获取进程命令行
		char cmdline[100] = { 0 };
		rwDriver.GetProcessCmdline(hProcess, cmdline, sizeof(cmdline));

		if (!!strstr(cmdline, "calc")) //将计算器进程提升至ROOT
		{
			//驱动_提升进程权限到Root
			BOOL b = rwDriver.SetProcessRoot(hProcess);
			printf("调用驱动 SetProcessRoot 返回值:%d\n", b);
		}

		//驱动_关闭进程
		rwDriver.CloseHandle(hProcess);

		printf("pid:%d,rss:%" PRIu64 ",cmdline:%s\n", pid, outRss, cmdline);
	}


	fflush(stdout);
	return 0;
}
