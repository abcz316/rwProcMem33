#include <cstdio>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <memory>
#include <vector>
#include <dirent.h>
#include <cinttypes>
#include <algorithm>
#include "MemoryReaderWriter39.h"
#include "../../testMemSearch/jni/MapRegionType.h"


int main(int argc, char *argv[]) {
	printf(
		"======================================================\n"
		"本驱动名称: Linux ARM64 硬件读写进程内存驱动39\n"
		"本驱动接口列表：\n"
		"\t1.	驱动_打开进程: OpenProcess\n"
		"\t2.	驱动_读取进程内存: ReadProcessMemory\n"
		"\t3.	驱动_写入进程内存: WriteProcessMemory\n"
		"\t4.	驱动_关闭进程: CloseHandle\n"
		"\t5.	驱动_获取进程内存块列表: VirtualQueryExFull（可选：显示全部内存、仅显示物理内存）\n"
		"\t6.	驱动_获取进程PID列表: GetPidList\n"
		"\t7.	驱动_提升进程权限到Root: SetProcessRoot\n"
		"\t8.	驱动_获取进程物理内存占用大小: GetProcessPhyMemSize\n"
		"\t9.	驱动_获取进程命令行: GetProcessCmdline\n"
		"\t10.	驱动_隐藏驱动: HideKernelModule\n"
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

	//驱动默认隐蔽通信密匙
	std::string procNodeAuthKey = "e84523d7b60d5d341a7c4d1861773ecd";
	if (argc > 1) {
		//用户自定义输入驱动隐蔽通信密匙
		procNodeAuthKey = argv[1];
	}
	printf("Connecting rwDriver auth key:%s\n", procNodeAuthKey.c_str());

	//连接驱动
	int err = rwDriver.ConnectDriver(procNodeAuthKey.c_str());
	if (err) {
		printf("Connect rwDriver failed. error:%d\n", err);
		fflush(stdout);
		return 0;
	}
	
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
	BOOL read_res = rwDriver.ReadProcessMemory(hProcess, (uint64_t)pBuf, &readBuf, sizeof(readBuf), &real_read, FALSE);
	printf("调用驱动 ReadProcessMemory 读取内存地址:%p,返回值:%d,读取到的内容:%s,实际读取大小:%zu\n", pBuf, read_res, readBuf, real_read);

	//驱动_写入进程内存
	snprintf(readBuf, sizeof(readBuf), "%s", "写入456");
	size_t real_write = 0;
	BOOL write_res = rwDriver.WriteProcessMemory(hProcess, (uint64_t)pBuf, &readBuf, sizeof(readBuf), &real_write, FALSE);
	printf("调用驱动 WriteProcessMemory 写入内存地址:%p,返回值:%d,写入的内容:%s,实际写入大小:%zu\n", pBuf, write_res, readBuf, real_write);
	printf("当前缓冲区内容 :%s,当前缓冲区的内存地址:%p\n", szBuf, pBuf);


	//驱动_获取进程内存块列表（显示全部内存）
	std::vector<DRIVER_REGION_INFO> vMaps;
	BOOL b = rwDriver.VirtualQueryExFull(hProcess, FALSE, vMaps);
	printf("调用驱动 VirtualQueryExFull(显示全部内存) 返回值:%d\n", b);

	//显示进程内存块地址列表
	for (DRIVER_REGION_INFO rinfo : vMaps) {
		printf("---Start:%p,Size:%" PRIu64 ",Type:%s,Name:%s\n", (void*)rinfo.baseaddress, rinfo.size, MapsTypeToString(&rinfo).c_str(), rinfo.name);
	}

	//驱动_获取进程内存块列表（仅显示物理内存）
	vMaps.clear();
	printf("正在调用驱动接口 VirtualQueryExFull（仅显示物理内存），该操作可能耗时较长，请耐心等待…\n");
	b = rwDriver.VirtualQueryExFull(hProcess, TRUE, vMaps);
	printf("调用驱动 VirtualQueryExFull(仅显示物理内存) 返回值:%d\n", b);

	//显示进程内存块地址列表
	for (DRIVER_REGION_INFO rinfo : vMaps) {
		printf("+++Start:%p,Size:%" PRIu64 ",Type:%s,Name:%s\n", (void*)rinfo.baseaddress, rinfo.size, MapsTypeToString(&rinfo).c_str(), rinfo.name);
	}

	//驱动_获取进程物理内存占用大小
	uint64_t outRss = 0;
	b = rwDriver.GetProcessPhyMemSize(hProcess, outRss);
	printf("调用驱动 GetProcessPhyMemSize 返回值:%d,当前进程物理内存占用大小:%" PRIu64 "KB\n", b, outRss);


	//驱动_获取进程命令行
	char cmdline[100] = { 0 };
	b = rwDriver.GetProcessCmdline(hProcess, cmdline, sizeof(cmdline));
	printf("调用驱动 GetProcessCmdline 返回值:%d,当前进程命令行:%s\n", b, cmdline);

	//驱动_关闭进程
	rwDriver.CloseHandle(hProcess);
	printf("调用驱动 CloseHandle %" PRIu64 "\n", hProcess);


	//驱动_获取进程PID列表
	std::vector<int> vPID;
	b = rwDriver.GetPidList(vPID);
	printf("调用驱动 GetPidList 返回值:%d, size:%zu\n", b, vPID.size());
	printf("打印进程列表信息(仅演示最后10个进程)\n");
	for(size_t i = (vPID.size() > 10 ? vPID.size() - 10 : 0); i < vPID.size(); ++i) {
		int pid = vPID[i];
		//驱动_打开进程
		uint64_t hProcess = rwDriver.OpenProcess(pid);
		if (!hProcess) { continue; }

		//驱动_获取进程物理内存占用大小
		uint64_t outRss = 0;
		rwDriver.GetProcessPhyMemSize(hProcess, outRss);

		//驱动_获取进程命令行
		char cmdline[100] = { 0 };
		rwDriver.GetProcessCmdline(hProcess, cmdline, sizeof(cmdline));

		if (!!strstr(cmdline, "calc")) { //将计算器进程提升至ROOT
			//驱动_提升进程权限到Root
			BOOL b = rwDriver.SetProcessRoot(hProcess);
			printf("调用驱动 SetProcessRoot 返回值:%d\n", b);
		}

		//驱动_关闭进程
		rwDriver.CloseHandle(hProcess);

		printf("pid:%d,rss:%" PRIu64 ",cmdline:%s\n", pid, outRss, cmdline);
	}

	//驱动_隐藏驱动
	b = rwDriver.HideKernelModule();
	printf("调用驱动 HideKernelModule 返回值:%d\n", b);

	fflush(stdout);
	return 0;
}