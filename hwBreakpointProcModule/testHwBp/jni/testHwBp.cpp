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
#include "HwBreakpointMgr4.h"

int g_value = 0;

BOOL GetProcessTask(int pid, std::vector<int> & vOutput) {
	DIR *dir = NULL;
	struct dirent *ptr = NULL;
	char szTaskPath[256] = { 0 };
	sprintf(szTaskPath, "/proc/%d/task", pid);

	dir = opendir(szTaskPath);
	if (NULL != dir) {
		while ((ptr = readdir(dir)) != NULL) {
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
	while(1) {
		g_value++;
		printf("pid:%d, g_value addr:%p, g_value: %d\n", getpid(), (void*)&g_value, g_value);
		fflush(stdout);
		sleep(1);
	}
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
		"本驱动名称: Linux ARM64 硬件断点进程调试驱动4\n"
		"本驱动接口列表：\n"
		"\t1.  驱动_打开进程: OpenProcess\n"
		"\t2.  驱动_关闭进程: CloseHandle\n"
		"\t3.  驱动_获取CPU硬件执行断点支持数量: GetNumBRPS\n"
		"\t4.  驱动_获取CPU硬件访问断点支持数量: GetNumWRPS\n"
		"\t5.  驱动_安装进程硬件断点: InstProcessHwBp\n"
		"\t6.  驱动_鞋子啊进程硬件断点: UninstProcessHwBp\n"
		"\t7.  驱动_暂停硬件断点: SuspendProcessHwBp\n"
		"\t8.  驱动_恢复硬件断点: ResumeProcessHwBp\n"
		"\t9.  驱动_读取硬件断点命中信息: ReadHwBpInfo\n"
		"\t10. 驱动_设置无条件Hook跳转: SetHookPC\n"
		"\t11. 驱动_隐藏驱动: HideKernelModule\n"
		"\t以上所有功能不注入、不附加进程, 不打开进程任何文件, 所有操作均为内核操作\n"
		"======================================================\n"
	);

	int pid = getpid();

	//启动数值增加线程
	std::thread tdValue(AddValueThread);
	tdValue.detach();
	usleep(1000*250);

	CHwBreakpointMgr driver;
	//驱动默认隐蔽通信密匙
	std::string procNodeAuthKey = "dce3771681d4c7a143d5d06b7d32548e";
	if (argc > 1) {
		//用户自定义输入驱动隐蔽通信密匙
		procNodeAuthKey = argv[1];
	}
	printf("Connecting HWBP auth key:%s\n", procNodeAuthKey.c_str());

	//连接驱动
	int err = driver.ConnectDriver(procNodeAuthKey.c_str());
	if (err < 0) {
		printf("Connect HWBP driver failed. error:%d\n", err);
		fflush(stdout);
		return 0;
	}

	//获取CPU支持硬件执行和访问断点的数量
	printf("Call GetNumBRPS() return:%d\n", driver.GetNumBRPS());
	printf("Call GetNumWRPS() return:%d\n", driver.GetNumWRPS());

	//获取当前进程所有的task
	std::vector<int> vTask;
	GetProcessTask(pid, vTask);
	if (vTask.size() == 0) {
		printf("GetProcessTask failed\n");
		return 0;
	}
	printf("GetProcessTask successfully, task count:%lu\n", vTask.size());

	//设置进程硬件断点
	std::vector<uint64_t> vHwBpHandle;
	for (int taskId : vTask) {
		if(taskId == getpid()) {
			continue; //不能打开自身
		}
		//打开task
		uint64_t hProcess = driver.OpenProcess(taskId);
		printf("Call OpenProcess(%d) return:%p\n", taskId, (void*)hProcess);
		if (!hProcess) {
			printf("Call OpenProcess failed\n");
			fflush(stdout);
			continue;
		}

		//设置读写硬件断点
		uint64_t hwBpHandle = driver.InstProcessHwBp(hProcess, (uint64_t)&g_value, HW_BREAKPOINT_LEN_4, HW_BREAKPOINT_W);
		printf("Call InstProcessHwBp(%d, %p) return:%p\n", taskId, &g_value, (void*)hwBpHandle);

		if (hwBpHandle) {
			vHwBpHandle.push_back(hwBpHandle);
		}
		//关闭task
		driver.CloseHandle(hProcess);
		printf("Call CloseHandle:%p\n", (void*)hProcess);
	}
	printf("==========================================================================\n");
	for(int i = 0; i < 5; i++) {
		sleep(1);
	}
	//暂停硬件断点
	for (uint64_t hwbpHandle : vHwBpHandle) {
		BOOL b = driver.SuspendProcessHwBp(hwbpHandle);
		printf("Call SuspendProcessHwBp(%p) return:%d\n", (void*)hwbpHandle, b);
	}
	//读取硬件断点命中信息
	for (uint64_t hwbpHandle : vHwBpHandle) {
		uint64_t nHitTotalCount = 0;
		std::vector<HW_HIT_ITEM> vHit;
		BOOL b = driver.ReadHwBpInfo(hwbpHandle, nHitTotalCount, vHit);
		printf("==========================================================================\n");
		printf("Call ReadProcessHwBp(%p) return:%d, hit_total_count:%lu, save_hit_item_count:%lu\n", (void*)hwbpHandle, b, nHitTotalCount, vHit.size());
		PrintHwHitItem(vHit);
	}
	//删除进程硬件断点
	for (uint64_t hwbpHandle : vHwBpHandle) {
		driver.UninstProcessHwBp(hwbpHandle);
		printf("Call UninstProcessHwBp(%p)\n", (void*)hwbpHandle);
	}
	return 0;
}