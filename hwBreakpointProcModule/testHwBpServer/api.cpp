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

/*
void CloseHandle(HANDLE h)
{
	int i;
	handleType ht = GetHandleType(h);
	uint64_t pl = GetPointerFromHandle(h);

	printf("CloseHandle %d %ld\n", h, pl);

	if (ht == htProcesHandle)
	{
		std::unique_ptr<CeOpenProcess> save((CeOpenProcess*)pl);
		g_Driver.CloseHandle(save->driverProcessHandle);
	}

}

HANDLE OpenProcess(DWORD pid)
{
	//check if this process has already been opened
	for (auto item = g_HandlePointList.begin(); item != g_HandlePointList.end(); item++)
	{
		if (item->second.type == htProcesHandle)
		{
			std::unique_ptr<CeOpenProcess> ceOpenProcess((CeOpenProcess*)item->second.p);
			if (ceOpenProcess->pid == pid)
			{
				ceOpenProcess.release();
				return item->first;
			}
			ceOpenProcess.release();
		}
	}
	//still here, so not opened yet

	//驱动_打开进程
	uint64_t driverProcessHandle = g_Driver.OpenProcess(pid);
	if (driverProcessHandle == 0)
	{
		return 0;
	}
	std::unique_ptr<CeOpenProcess> ceOpenProcessOpen = std::make_unique<CeOpenProcess>();
	ceOpenProcessOpen->pid = pid;
	ceOpenProcessOpen->driverProcessHandle = driverProcessHandle;
	return CreateHandleFromPointer((uint64_t)ceOpenProcessOpen.release(), htProcesHandle);
}
*/

void ProcessAddProcessHwBp(AddProcessHwBpInfo &params,
	int &allTaskCount, int &insHwBpSuccessTaskCount, std::vector<struct USER_HIT_INFO> &vHit) {
	allTaskCount = 0;
	insHwBpSuccessTaskCount = 0;
	//储存需要下断的线程列表
	std::vector<int> vTask;
	if (params.hwBpThreadType == 0) //硬件下断全部线程
	{
		//获取当前进程所有的task
		GetProcessTask(params.pid, vTask);

	} else if (params.hwBpThreadType == 1) //硬件下断主线程
	{
		vTask.push_back(params.pid);
	} else if (params.hwBpThreadType == 2) //硬件除主线程之外的其他线程
	{
		//获取当前进程所有的task
		GetProcessTask(params.pid, vTask);

		//删除主线程
		for (auto iter = vTask.begin(); iter != vTask.end(); iter++) {
			if (*iter == params.pid) {
				vTask.erase(iter);
				break;
			}
		}
	}

	allTaskCount = vTask.size();


	//硬件断点句柄
	std::vector<uint64_t> vHwBpHandle;
	for (int i = 0; i < vTask.size(); i++) {
		//打开task
		uint64_t hProcess = g_Driver.OpenProcess(vTask.at(i));
		if (hProcess) {
			//驱动_新增硬件断点，返回值：TRUE成功，FALSE失败
			uint64_t hwBpHandle = g_Driver.AddProcessHwBp(
				hProcess, params.address,
				params.hwBpAddrLen, params.hwBpAddrType);

			if (hwBpHandle) {
				vHwBpHandle.push_back(hwBpHandle);

				printf("result of AddProcessHwBp=%" PRIu64 "\n", hwBpHandle);


			}

		}

	}
	insHwBpSuccessTaskCount = vHwBpHandle.size();

	if (vHwBpHandle.size()) {
		//有成功的下断
		usleep(params.hwBpKeepTimeMs * 1000); //delay ms

		//删除进程硬件断点
		for (uint64_t hwBpHandle : vHwBpHandle) {
			g_Driver.DelProcessHwBp(hwBpHandle);
			printf("Call DelProcessHwBp(%" PRIu64 ")\n", hwBpHandle);
		}

		//读取硬件断点命中信息
		for (uint64_t hwBpHandle : vHwBpHandle) {
			BOOL b = g_Driver.ReadHwBpInfo(hwBpHandle, vHit);
			printf("Call ReadProcessHwBp(%" PRIu64 ") %d\n", hwBpHandle, b);
		}
		//清空硬件断点命中信息
		g_Driver.CleanHwBpInfo();
		printf("Call CleanHwBpInfo()\n");
	}

	return;
}

int ProcessSetHwBpHitConditions(HIT_CONDITIONS params) {
	//驱动_设置硬件断点命中记录条件，返回值：TRUE成功，FALSE失败
	return g_Driver.SetHwBpHitConditions(params);
}
