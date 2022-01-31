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
#include "HwBreakpointManager.h"


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


void AddValueThread() {
	while (1) {
		g_value++;
		printf("pid:%d, g_value addr:0x%llx, g_value: %d\n", getpid(), &g_value, g_value);
		fflush(stdout);
		sleep(1);
	}
	return;
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

	int pid = getpid();


	//启动数值增加线程
	std::thread tdValue(AddValueThread);
	tdValue.detach();

	CHwBreakpointManager driver;
	//连接驱动
	int err = 0;
	if (!driver.ConnectDriver(err)) {
		printf("连接驱动失败\n");
		return 0;
	}



	//获取CPU支持硬件执行和访问断点的数量
	int bprsCount = driver.GetNumBRPS();
	int wprsCount = driver.GetNumWRPS();
	printf("驱动_获取CPU支持硬件执行断点的数量:%d\n", bprsCount);
	printf("驱动_获取CPU支持硬件访问断点的数量:%d\n", wprsCount);



	//获取当前进程所有的task
	std::vector<int> vTask;
	GetProcessTask(pid, vTask);
	if (vTask.size() == 0) {
		printf("获取当前进程task失败\n");
		return 0;
	}


	//设置进程硬件断点
	std::vector<uint64_t> vHwBpHandle;

	for (int task : vTask) {
		//打开task
		uint64_t hProcess = driver.OpenProcess(task);
		printf("调用驱动 OpenProcess(%d) 返回值:%" PRIu64 "\n", task, hProcess);
		if (!hProcess) {
			printf("调用驱动 OpenProcess 失败\n");
			fflush(stdout);
			continue;
		}

		//设置进程硬件断点
		uint64_t hwBpHandle = driver.AddProcessHwBp(hProcess, (uint64_t)&g_value,
			HW_BREAKPOINT_LEN_4, HW_BREAKPOINT_W);
		printf("调用驱动 AddProcessHwBp(%p) 返回值:%" PRIu64 "\n", &g_value, hwBpHandle);


		if (hwBpHandle) {
			vHwBpHandle.push_back(hwBpHandle);
		}
		//关闭task
		driver.CloseHandle(hProcess);
		printf("调用驱动 CloseHandle:%" PRIu64 "\n", hProcess);
	}
	sleep(5);
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
		printf("调用驱动 ReadProcessHwBp(%" PRIu64 ") 返回值:%d\n", hwBpHandle, b);
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
	printf("调用驱动 CleanHwBpInfo()\n");
	return 0;
}