#include <cstdio>
#include <string.h> 
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <memory>
#include <sstream>
#include <cinttypes>
#include <iostream>
#include <map>
#include <sys/stat.h>
#include <zlib.h>
#include "libzip/zip.h"
#include "../testKo/MemoryReaderWriter37.h"
#include "../testMemSearch/MapRegionType.h"


std::string& replace_all_distinct(std::string& str, const std::string& old_value, const std::string& new_value) {
	for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()) {
		if ((pos = str.find(old_value, pos)) != std::string::npos) {
			str.replace(pos, old_value.length(), new_value);
		} else {
			break;
		}
	}
	return str;
}

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


	//写出数据到压缩包文件
	const char* lpszOutFilePath = "/sdcard/dumptest.zip";

	//新建压缩包文件
	err = 0;
	zip *z = zip_open(lpszOutFilePath, ZIP_CREATE | ZIP_EXCL, &err);
	if (!z) {
		printf("zip_open failed.\n");
		return 0;
	}


	//驱动_获取进程内存块列表（只显示在物理内存中的内存）
	std::vector<DRIVER_REGION_INFO> vMaps;
	BOOL bOutListCompleted;
	BOOL b = rwDriver.VirtualQueryExFull(hProcess, TRUE, vMaps, bOutListCompleted);
	printf("调用驱动 VirtualQueryExFull(只显示在物理内存中的内存) 返回值:%d\n", b);
	if (!vMaps.size()) {
		printf("VirtualQueryExFull 失败\n");

		//关闭进程
		rwDriver.CloseHandle(hProcess);
		printf("调用驱动 CloseHandle:%" PRIu64 "\n", hProcess);
		fflush(stdout);
		return 0;
	}


	//开始生成指针映射集
	std::vector<std::shared_ptr<char>> vspMemData;
	size_t total_size = 0;
	for (DRIVER_REGION_INFO rinfo : vMaps) {
		printf("[%.2f%%][%zd Mb] +++Start:%p,Size:%" PRIu64 ",Type:%s,Name:%s\n",
			(float)((float)vspMemData.size() * 100 / vMaps.size()),
			total_size / 1024 / 1024,
			(void*)rinfo.baseaddress, rinfo.size,
			MapsTypeToString(&rinfo).c_str(), rinfo.name);

		//申请内存
		std::shared_ptr<char> spMem(new char[rinfo.size], [](char *p) { delete[] p; });
		if (!spMem) {
			printf("malloc(%" PRIu64 ") failed.\n", rinfo.size);
			continue;
		}
		memset(spMem.get(), 0, rinfo.size);
		total_size += rinfo.size;


		//读取进程内存
		size_t real_read;
		auto read_res = rwDriver.ReadProcessMemory(hProcess, rinfo.baseaddress, spMem.get(), rinfo.size, &real_read, FALSE);
		printf("ReadProcessMemory(%p)=%d, %zu\n", (void*)rinfo.baseaddress, read_res, real_read);

		//压缩包里的文件名
		std::stringstream ssfilename;
		ssfilename << "/" << std::hex << (void*)rinfo.baseaddress;
		ssfilename << "_";
		ssfilename << rinfo.size;
		ssfilename << "_";
		ssfilename << MapsTypeToString(&rinfo);
		if (rinfo.name[0] != '\0') {
			//替换斜杠
			std::string specialFileName = rinfo.name;
			specialFileName = replace_all_distinct(specialFileName, "/", "／");
			ssfilename << "_" << specialFileName;
		}

		//添加进压缩包
		struct zip_source *s = zip_source_buffer(z, spMem.get(), rinfo.size, 0);
		if (!s) {
			printf("zip_source_buffer failed.\n");
			continue;
		}

		zip_file_add(z, ssfilename.str().c_str(), s,
			ZIP_FL_OVERWRITE | ZIP_FL_ENC_GUESS);

		//would be used and freed by zip_close(),
		//so don't free the zip_source here.
		//zip_source_free(s); 

		vspMemData.push_back(spMem);
	}

	//把内存页大小也添加进压缩包
	char empty = '\0';
	struct zip_source *s = zip_source_buffer(z, &empty, sizeof(empty), 0);
	if (s) {
		std::stringstream ssfilename;
		ssfilename << "/0x0_" << std::hex << getpagesize() << "_----_pagesize";
		zip_file_add(z, ssfilename.str().c_str(), s,
			ZIP_FL_OVERWRITE | ZIP_FL_ENC_GUESS);
	}

	//关闭压缩包
	printf("zip_closing...\n");
	err = zip_close(z);
	printf("zip_close done.\n");
	return 0;
}
