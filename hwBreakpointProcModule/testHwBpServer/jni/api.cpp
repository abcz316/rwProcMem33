#include "api.h"
#include <malloc.h>
#include <sstream>
#include <memory>
#include <random>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <cinttypes>
#include "hwbpserver.h"

//获取当前进程所有的task
BOOL GetProcessTask(int pid, std::vector<int>& vOutput) {
	DIR* dir = NULL;
	struct dirent* ptr = NULL;
	char szTaskPath[256] = { 0 };
	sprintf(szTaskPath, "/proc/%d/task", pid);

	dir = opendir(szTaskPath);
	if (NULL != dir) {
		while ((ptr = readdir(dir)) != NULL)   // 循环读取路径下的每一个文件/文件夹
		{
			// 如果读取到的是"."或者".."则跳过，读取到的不是文件夹名字也跳过
			if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0)) {
				continue;
			}
			else if (ptr->d_type != DT_DIR) {
				continue;
			}
			else if (strspn(ptr->d_name, "1234567890") != strlen(ptr->d_name)) {
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


void ProcessInstProcessHwBp(const struct InstProcessHwBpInfo& input, struct InstProcessHwBpResult & output) {
	output.allTaskCount = 0;
	output.hwbpInstalledCount = 0;
	//储存需要下断的线程列表
	std::vector<int> vTask;
	if (input.hwBpThreadType == 0) { //硬件下断全部线程
		//获取当前进程所有的task
		GetProcessTask(input.pid, vTask);
	}
	else if (input.hwBpThreadType == 1) {//硬件下断主线程
		vTask.push_back(input.pid);
	}
	else if (input.hwBpThreadType == 2) { //硬件除主线程之外的其他线程
		//获取当前进程所有的task
		GetProcessTask(input.pid, vTask);

		//删除主线程
		for (auto iter = vTask.begin(); iter != vTask.end(); iter++) {
			if (*iter == input.pid) {
				vTask.erase(iter);
				break;
			}
		}
	}

	output.allTaskCount = vTask.size();

	//硬件断点句柄
	std::vector<uint64_t> vHwBpHandle;
	for (int taskId : vTask) {
		//打开task
		uint64_t hProcess = g_driver.OpenProcess(taskId);
		if (!hProcess) {
			continue;
		}
		//设置读写硬件断点
		uint64_t hwBpHandle = g_driver.InstProcessHwBp(hProcess, input.address, input.hwBpAddrLen, input.hwBpAddrType);
		printf("Call InstProcessHwBp(%d, %p) return:%p\n", taskId, (void*)input.address, (void*)hwBpHandle);
		if (hwBpHandle) {
			vHwBpHandle.push_back(hwBpHandle);
		}
		//关闭task
		g_driver.CloseHandle(hProcess);
		printf("Call CloseHandle:%p\n", (void*)hProcess);
	}
	output.hwbpInstalledCount = vHwBpHandle.size();
	if (output.hwbpInstalledCount) {
		//有成功的下断
		usleep(input.hwBpKeepTimeMs * 1000); //delay ms
		//暂停硬件断点
		for (uint64_t hwbpHandle : vHwBpHandle) {
			BOOL b = g_driver.SuspendProcessHwBp(hwbpHandle);
			printf("Call SuspendProcessHwBp(%p) return:%d\n", (void*)hwbpHandle, b);
		}
		//读取硬件断点命中信息
		for (uint64_t hwbpHandle : vHwBpHandle) {
			InstProcessHwBpResultChild threadHitInfo;
			BOOL b = g_driver.ReadHwBpInfo(hwbpHandle, threadHitInfo.hitTotalCount, threadHitInfo.vHitItem);
			printf("Call ReadProcessHwBp(%p) return:%d, hit_total_count:%lu, save_hit_item_count:%lu\n", (void*)hwbpHandle, b, threadHitInfo.hitTotalCount, threadHitInfo.vHitItem.size());
			if(!b || !threadHitInfo.vHitItem.size()) {
				continue;
			}
			threadHitInfo.taskId = threadHitInfo.vHitItem[0].task_id;
			threadHitInfo.address = input.address;
			output.vThreadHit.push_back(threadHitInfo);
		}

		//删除进程硬件断点
		for (uint64_t hwbpHandle : vHwBpHandle) {
			g_driver.UninstProcessHwBp(hwbpHandle);
			printf("Call UninstProcessHwBp(%p)\n", (void*)hwbpHandle);
		}
	}
}

