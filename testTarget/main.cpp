#include <cstdio>
#include <string.h> 
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <memory>
#include <dirent.h>
#include <cinttypes>
#include "../testKo/MemoryReaderWriter37.h"
#include "../testMemSearch/MapRegionType.h"




int findPID(const char *lpszCmdline, CMemoryReaderWriter *pDriver) {
	int nTargetPid = 0;

	//驱动_获取进程PID列表
	std::vector<int> vPID;
	BOOL bOutListCompleted;
	BOOL b = pDriver->GetProcessPidList(vPID, FALSE, bOutListCompleted);
	printf("调用驱动 GetProcessPidList 返回值:%d\n", b);

	//打印进程列表信息
	for (int pid : vPID) {
		//驱动_打开进程
		uint64_t hProcess = pDriver->OpenProcess(pid);
		if (!hProcess) { continue; }

		//驱动_获取进程命令行
		char cmdline[100] = { 0 };
		pDriver->GetProcessCmdline(hProcess, cmdline, sizeof(cmdline));

		//驱动_关闭进程
		pDriver->CloseHandle(hProcess);

		if (strcmp(lpszCmdline, cmdline) == 0) {
			nTargetPid = pid;
			break;
		}
	}
	return nTargetPid;
}




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


	//获取目标进程PID
	const char *name = "com.miui.calculator";
	pid_t pid = findPID(name, &rwDriver);
	if (pid == 0) {
		printf("找不到进程\n");
		return 0;
	}
	printf("目标进程PID:%d\n", pid);

	//打开进程
	uint64_t hProcess = rwDriver.OpenProcess(pid);
	printf("调用驱动 OpenProcess 返回值:%" PRIu64 "\n", hProcess);
	if (!hProcess) {
		printf("调用驱动 OpenProcess 失败\n");
		fflush(stdout);
		return 0;
	}

	//驱动_获取进程内存块列表（显示全部内存）
	std::vector<DRIVER_REGION_INFO> vMaps;
	BOOL bOutListCompleted;
	BOOL b = rwDriver.VirtualQueryExFull(hProcess, FALSE, vMaps, bOutListCompleted);
	printf("调用驱动 VirtualQueryExFull(显示全部内存) 返回值:%d\n", b);

	if (!vMaps.size()) {
		printf("VirtualQueryExFull 失败\n");

		//关闭进程
		rwDriver.CloseHandle(hProcess);
		printf("调用驱动 CloseHandle:%" PRIu64 "\n", hProcess);
		fflush(stdout);
		return 0;
	}

	//显示进程内存块地址列表
	for (DRIVER_REGION_INFO rinfo : vMaps) {
		printf("---Start:%p,Size:%" PRIu64 ",Type:%s,Name:%s\n", (void*)rinfo.baseaddress, rinfo.size, MapsTypeToString(&rinfo).c_str(), rinfo.name);
	}


	//驱动_获取进程内存块列表（只显示在物理内存中的内存）
	vMaps.clear();
	b = rwDriver.VirtualQueryExFull(hProcess, TRUE, vMaps, bOutListCompleted);
	printf("调用驱动 VirtualQueryExFull(只显示在物理内存中的内存) 返回值:%d\n", b);
	if (!vMaps.size()) {
		printf("VirtualQueryExFull 失败\n");

		//关闭进程
		rwDriver.CloseHandle(hProcess);
		printf("调用驱动 CloseHandle:%" PRIu64 "\n", hProcess);
		fflush(stdout);
		return 0;
	}
	//显示进程内存块地址列表
	for (DRIVER_REGION_INFO rinfo : vMaps) {
		printf("+++Start:%p,Size:%" PRIu64 ",Type:%s,Name:%s\n", (void*)rinfo.baseaddress, rinfo.size, MapsTypeToString(&rinfo).c_str(), rinfo.name);
	}


	//获取一个可读地址
	uint64_t pRead = 0;
	size_t nReadSize = 0;

	for (DRIVER_REGION_INFO rinfo : vMaps) {
		if (rinfo.protection != PAGE_NOACCESS) //此地址可以访问
		{
			//记录一个可以访问的地址信息
			pRead = rinfo.baseaddress;
			nReadSize = rinfo.size;
			break;
		}
	}

	if (pRead == 0) //没找到可以访问的地址
	{
		printf("找不到可读地址\n");

		//关闭进程
		rwDriver.CloseHandle(hProcess);
		printf("调用驱动 CloseHandle:%" PRIu64 "\n", hProcess);
		fflush(stdout);
		return 0;
	}

	//开始演示读取进程内存
	size_t real_read = 0;

	nReadSize = nReadSize > 1024 ? 1024 : nReadSize;

	char *readBuf = (char*)malloc(nReadSize);

	auto read_res = rwDriver.ReadProcessMemory(hProcess, pRead, readBuf, nReadSize, &real_read, FALSE);
	printf("调用驱动 ReadProcessMemory 读取内存地址:%p,返回值:%d,实际读取大小:%zu \n", (void*)pRead, read_res, real_read);

	for (int i = 0; i < nReadSize; i++) {
		char c = readBuf[i];
		printf("%x ", c);
	}
	printf("\n");

	/*
	//开始演示写入进程内存
	memset(readBuf, 0, sizeof(readBuf));
	snprintf(readBuf, sizeof(readBuf), "%s", "写入456");
	size_t real_write = 0;
	auto write_res = rwDriver.WriteProcessMemory(hProcess, (uint64_t)pBuf, &readBuf, sizeof(readBuf), &real_write, FALSE);
	printf("调用驱动 WriteProcessMemory 写入内存地址:%p,返回值:%d,写入的内容:%s,实际写入大小:%zu\n", pBuf, write_res, readBuf, real_write);
	*/

	free(readBuf);

	//关闭进程
	rwDriver.CloseHandle(hProcess);
	printf("调用驱动 CloseHandle:%" PRIu64 "\n", hProcess);
	fflush(stdout);
	return 0;
}