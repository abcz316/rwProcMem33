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

#include "../../testHwBp/jni/HwBreakpointMgr3.h"

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

std::string TimestampToDatetime(uint64_t timestamp) {
    time_t t = (time_t)timestamp;
    struct tm *tm_info = localtime(&t);
    char buffer[256] = {0};
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

void PrintHwHitItem(const std::vector<HW_HIT_ITEM> &vHit) {
	for (const HW_HIT_ITEM &hitItem : vHit) {
		printf("==========================================================================\n");
		printf("时间:%s, 线程ID:%zu, 硬断命中地址:%p\n", TimestampToDatetime(hitItem.hit_time).c_str(), hitItem.task_id, (void*)hitItem.hit_addr);
		for (int r = 0; r < 30; r += 5) {
			printf("\tX%-2d=%-20p, X%-2d=%-20p, X%-2d=%-20p, X%-2d=%-20p, X%-2d=%-20p\n",
				r, (void*)hitItem.regs_info.regs[r],
				r + 1, (void*)hitItem.regs_info.regs[r + 1],
				r + 2, (void*)hitItem.regs_info.regs[r + 2],
				r + 3, (void*)hitItem.regs_info.regs[r + 3],
				r + 4, (void*)hitItem.regs_info.regs[r + 4]);
		}
		printf("\tLR=%-20p, SP=%-20p, PC=%-20p\n",
			(void*)hitItem.regs_info.regs[30],
			(void*)hitItem.regs_info.sp,
			(void*)hitItem.regs_info.pc);

		printf("\tprocess status:%p, orig_x0:%p, syscallno:%p\n",
			(void*)hitItem.regs_info.pstate,
			(void*)hitItem.regs_info.orig_x0,
			(void*)hitItem.regs_info.syscallno);
	}
}


int main(int argc, char *argv[]) {
	printf(
		"======================================================\n"
		"本驱动名称: Linux ARM64 硬件断点进程调试驱动3\n"
		"本驱动接口列表：\n"
		"\t1.  驱动_打开进程: OpenProcess\n"
		"\t2.  驱动_关闭进程: CloseHandle\n"
		"\t3.  驱动_获取CPU支持硬件执行断点的数量: GetNumBRPS\n"
		"\t4.  驱动_获取CPU支持硬件访问断点的数量: GetNumWRPS\n"
		"\t5.  驱动_设置进程硬件断点: AddProcessHwBp\n"
		"\t6.  驱动_删除进程硬件断点: DelProcessHwBp\n"
		"\t7.  驱动_暂停硬件断点: SuspendProcessHwBp\n"
		"\t8.  驱动_恢复硬件断点: ResumeProcessHwBp\n"
		"\t9.  驱动_读取硬件断点命中信息: ReadHwBpInfo\n"
		"\t以上所有功能不注入、不附加进程, 不打开进程任何文件, 所有操作均为内核操作\n"
		"======================================================\n"
	);

	int opt;
	int argv_pid = 0;
	size_t argv_hwbp_addr = 0;
	int argv_hwbp_len = 0;
	int argv_hwbp_type = 0;
	int argv_sleep_time = 0;

	while ((opt = getopt(argc, argv, "p:a:l:t:s:")) != -1) {
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
		case 's':
			argv_sleep_time = atoi(optarg);
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
	printf("pid:%d,addr:%p,len:%d,type:%d\n", argv_pid, (void*)argv_hwbp_addr, argv_hwbp_len, argv_hwbp_type);


	CHwBreakpointMgr driver;
	//连接驱动
	int err = 0;
	if (!driver.ConnectDriver(HWBP_FILE_NODE, FALSE, err)) {
		printf("Connect HWBP driver failed. error:%d\n", err);
		fflush(stdout);
		return 0;
	}

	//获取CPU支持硬件执行和访问断点的数量
	int brpsCount = driver.GetNumBRPS();
	int wrpsCount = driver.GetNumWRPS();
	printf("Call GetNumBRPS() return:%d\n", brpsCount);
	printf("Call GetNumWRPS() return:%d\n", wrpsCount);


	//获取目标进程所有的task
	std::vector<int> vTask;
	GetProcessTask(argv_pid, vTask);
	if (vTask.size() == 0) {
		printf("GetProcessTask failed\n");
		return 0;
	}
	printf("==============================AddProcessHwBp===============================\n");
	//设置进程硬件断点
	std::vector<uint64_t> vHwBpHandle;
	for (int taskId : vTask) {
		//打开task
		uint64_t hProcess = driver.OpenProcess(taskId);
		if (!hProcess) {
			printf("Call OpenProcess(%d) failed.\n", taskId);
			fflush(stdout);
			continue;
		}

		//设置进程硬件断点
		uint64_t hwBpHandle = driver.AddProcessHwBp(hProcess, argv_hwbp_addr,
			argv_hwbp_len, argv_hwbp_type);
		if (hwBpHandle) {
			vHwBpHandle.push_back(hwBpHandle);
		} else {
			printf("Call AddProcessHwBp(%d) failed.\n", taskId);
		}
		//关闭task
		driver.CloseHandle(hProcess);
	}
	printf("GetProcessTask successfully, task count:%lu, install successfully count:%lu, hw addr:%p, len:%d, type:%d, sleep:%d\n", 
		vTask.size(), vHwBpHandle.size(), (void*)argv_hwbp_addr, argv_hwbp_len, argv_hwbp_type, argv_sleep_time);

	sleep(std::max(argv_sleep_time, 3));
	printf("==============================SuspendProcessHwBp=============================\n");
	//暂停硬件断点
	for (uint64_t hwbpHandle : vHwBpHandle) {
		driver.SuspendProcessHwBp(hwbpHandle);
	}
	//读取硬件断点命中信息
	for (uint64_t hwbpHandle : vHwBpHandle) {
		uint64_t nHitTotalCount = 0;
		std::vector<HW_HIT_ITEM> vHit;
		BOOL b = driver.ReadHwBpInfo(hwbpHandle, nHitTotalCount, vHit);
		if(nHitTotalCount == 0) {
			continue;
		}
		printf("============================ReadProcessHwBp===============================\n");
		printf("Call ReadProcessHwBp(%p) return:%d, hit_total_count:%lu, save_hit_item_count:%lu\n", (void*)hwbpHandle, b, nHitTotalCount, vHit.size());
		PrintHwHitItem(vHit);
	}
	printf("==============================DelProcessHwBp================================\n");
	//删除进程硬件断点
	for (uint64_t hwbpHandle : vHwBpHandle) {
		driver.DelProcessHwBp(hwbpHandle);
	}
	printf("done.\n");
	return 0;
}