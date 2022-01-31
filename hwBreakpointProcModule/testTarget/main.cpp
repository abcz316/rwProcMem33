#include <cstdio>
#include <string.h> 
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <memory>
#include <dirent.h>
#include <thread>
#include <sstream>
#include <cinttypes>

#include "../testHwBp/HwBreakpointManager.h"


//启用驱动KEY验证系统
#define CONFIG_VERIFY
#ifdef CONFIG_VERIFY
#include "../testHwBp/GetVerifyKey.h"
#endif

int g_value = 0;

BOOL GetProcessTask(int pid, std::vector<int> & vOutput) {
	DIR *dir = NULL;
	struct dirent *ptr = NULL;
	char szTaskPath[256] = { 0 };
	sprintf(szTaskPath, "/proc/%d/task", pid);

	dir = opendir(szTaskPath);
	if (NULL != dir) {
		while ((ptr = readdir(dir)) != NULL)   // 循环读取路径下的每一个文件/文件夹
		{
			// 如果读取到的是"."或者".."则跳过，读取到的不是文件夹名字也跳过
			if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0)) {
				continue;
			} else if (ptr->d_type != DT_DIR) {
				continue;
			} else if (strspn(ptr->d_name, "1234567890") != strlen(ptr->d_name)) {
				continue;
			}

			int task = atoi(ptr->d_name);
			vOutput.push_back(task);
		}
		closedir(dir);
		return TRUE;
	}
	return FALSE;
}


int main(int argc, char *argv[]) {
	printf(
		"======================================================\n"
		"本驱动名称: Linux ARM64 硬件断点进程调试驱动1\n"
		"本驱动接口列表：\n"
		"\t1.  驱动_打开进程: OpenProcess\n"
		"\t2.  驱动_获取CPU支持硬件执行断点的数量: GetNumBRPS\n"
		"\t3.  驱动_获取CPU支持硬件访问断点的数量: GetNumWRPS\n"
		"\t4.  驱动_设置进程硬件断点: AddProcessHwBp\n"
		"\t5.  驱动_删除进程硬件断点: DelProcessHwBp\n"
		"\t6.  驱动_读取硬件断点命中信息: ReadHwBpInfo\n"
		"\t7.  驱动_清空硬件断点命中信息: CleanHwBpInfo\n"
		"\t8.  驱动_关闭进程: CloseHandle\n"
		"\t以上所有功能不注入、不附加进程，不打开进程任何文件，所有操作均为硬件操作\n"
		"======================================================\n"
	);


	int opt;
	int argv_pid = 0;
	size_t argv_hwbp_addr = 0;
	int argv_hwbp_len = 0;
	int argv_hwbp_type = 0;

	while ((opt = getopt(argc, argv, "p:a:l:t:")) != -1) {
		switch (opt) {
		case 'p':
			argv_pid = atoi(optarg);
			break;
		case 'a':
			argv_hwbp_addr = strtoul(optarg, NULL, 16);
			break;
		case 'l':
			argv_hwbp_len = atoi(optarg);
			break;
		case 't':
			if (strcmp(optarg, "r") == 0) {
				argv_hwbp_type = HW_BREAKPOINT_R;
			} else if (strcmp(optarg, "w") == 0) {
				argv_hwbp_type = HW_BREAKPOINT_W;
			} else if (strcmp(optarg, "rw") == 0) {
				argv_hwbp_type = HW_BREAKPOINT_R | HW_BREAKPOINT_W;
			} else if (strcmp(optarg, "x") == 0) {
				argv_hwbp_type = HW_BREAKPOINT_X;
			}
			break;
		default:
			printf("Usage: %s [-p <attach_pid>] [-a <memory_hex_addr>] [-l <hw_breakpoint_len>] [-t <hw_breakpoint_type>] arg1 ...\n", argv[0]);
			printf("Example: %s -p 8072 -a 9ECF6140 -l 8 -t rw\n", argv[0]);
			return 0;
			break;
		}
	}
	if (argv_pid == 0 || argv_hwbp_addr == 0 || argv_hwbp_len == 0 || argv_hwbp_type == 0) {
		printf("Error value.\n");
		return 0;
	}
	printf("pid:%d,addr:%p,len:%d,type:%d\n", argv_pid, argv_hwbp_addr, argv_hwbp_len, argv_hwbp_type);


	CHwBreakpointManager driver;
	//连接驱动
	int err = 0;
	if (!driver.ConnectDriver(err)) {
		printf("连接驱动失败\n");
		return 0;
	}



#ifdef CONFIG_VERIFY
	//驱动_设置密匙（如果驱动开启了密匙验证系统，则需要输入密匙才可读取其他进程内存，否则只能读取自身进程）
	char identityBuf128[128] = { 0 };
	if (driver.SetKey((char*)&identityBuf128, 0)) {
		printf("Driver verify identity:%" PRIu64 "\n", *(uint64_t*)&identityBuf128);

		uint64_t key = GetVerifyKey((char*)&identityBuf128, sizeof(identityBuf128));
		printf("Driver verify key:%" PRIu64 "\n", key);

		if (driver.SetKey(NULL, key)) {
			printf("Driver verify key success.\n");
		} else {
			printf("Driver verify key failed.\n");
		}
	} else {
		printf("Get driver verify question failed.\n");
	}
#endif



	//获取CPU支持硬件执行和访问断点的数量
	int bprsCount = driver.GetNumBRPS();
	int wprsCount = driver.GetNumWRPS();
	printf("驱动_获取CPU支持硬件执行断点的数量:%d\n", bprsCount);
	printf("驱动_获取CPU支持硬件访问断点的数量:%d\n", wprsCount);



	//获取当前进程所有的task
	std::vector<int> vTask;
	GetProcessTask(argv_pid, vTask);
	if (vTask.size() == 0) {
		printf("获取目标进程task失败\n");
		return 0;
	}
	printf("task count:%d\n", vTask.size());

	//设置进程硬件断点
	std::vector<uint64_t> vHwBpHandle;

	for (int i = 0; i < vTask.size(); i++) {
		//打开task
		uint64_t hProcess = driver.OpenProcess(vTask.at(i));
		printf("调用驱动 OpenProcess(%d) 返回值:%" PRIu64 "\n", vTask.at(i), hProcess);
		if (!hProcess) {
			printf("调用驱动 OpenProcess 失败\n");
			fflush(stdout);
			continue;
		}

		//设置进程硬件断点
		uint64_t hwBpHandle = driver.AddProcessHwBp(hProcess, argv_hwbp_addr,
			argv_hwbp_len, argv_hwbp_type);
		printf("调用驱动 AddProcessHwBp(%p) 返回值:%" PRIu64 "\n", &g_value, hwBpHandle);

		if (hwBpHandle) {
			vHwBpHandle.push_back(hwBpHandle);
		}
		//关闭task
		driver.CloseHandle(hProcess);
		printf("调用驱动 CloseHandle:%" PRIu64 "\n", hProcess);


	}
	sleep(2);
	printf("==========================================================================\n");
	//删除进程硬件断点
	for (uint64_t hwBpHandle : vHwBpHandle) {
		driver.DelProcessHwBp(hwBpHandle);
		printf("调用驱动 DelProcessHwBp(%" PRIu64 ")\n", hwBpHandle);
	}
	//读取硬件断点命中信息
	for (uint64_t hwBpHandle : vHwBpHandle) {
		std::vector<USER_HIT_INFO> vHit;
		BOOL b = driver.ReadHwBpInfo(hwBpHandle, vHit);
		printf("==========================================================================\n");
		printf("Call ReadProcessHwBp(%" PRIu64 ") 返回值:%d\n", hwBpHandle, b);
		for (USER_HIT_INFO userhInfo : vHit) {
			printf("==========================================================================\n");
			printf("硬件断点命中地址:%p,命中次数:%zu\n", userhInfo.hit_addr, userhInfo.hit_count);
			for (int r = 0; r < 30; r += 5) {
				printf("\tX%-2d=%-12llx X%-2d=%-12llx X%-2d=%-12llx X%-2d=%-12llx X%-2d=%-12llx\n",
					r, userhInfo.regs.regs[r],
					r + 1, userhInfo.regs.regs[r + 1],
					r + 2, userhInfo.regs.regs[r + 2],
					r + 3, userhInfo.regs.regs[r + 3],
					r + 4, userhInfo.regs.regs[r + 4]);
			}
			printf("\tLR=%-12llx SP=%-12llx PC=%-12llx\n",
				userhInfo.regs.regs[30],
				userhInfo.regs.sp,
				userhInfo.regs.pc);

			printf("\tprocess status:%-12llx orig_x0:%-12llx syscallno:%-12llx\n",
				userhInfo.regs.pstate,
				userhInfo.regs.orig_x0,
				userhInfo.regs.syscallno);
			printf("==========================================================================\n");
		}
	}
	//清空硬件断点命中信息
	driver.CleanHwBpInfo();
	printf("Call CleanHwBpInfo()\n");
	return 0;
}