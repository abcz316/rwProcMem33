#include "api.h"
#include <malloc.h>
#include <sstream>
#include <memory>
#include <random>
#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <cinttypes>
#include "ceserver.h"


CMemoryReaderWriter m_Driver;



BOOL CApi::InitReadWriteDriver(const char* lpszDevFileName) {
	if (!lpszDevFileName) {
		//驱动默认文件名
		lpszDevFileName = RWPROCMEM_FILE_NODE;
	}

	printf("Connecting rwDriver:%s\n", lpszDevFileName);


	//连接驱动
	int err = 0;
	if (!m_Driver.ConnectDriver(lpszDevFileName, err)) {
		printf("Connect rwDriver failed. error:%d\n", err);
		fflush(stdout);
		return FALSE;
	}


	m_Driver.SetMaxDevFileOpen(100);
	return TRUE;

}



BOOL GetProcessListInfo(CMemoryReaderWriter* pDriver, BOOL bGetPhyMemorySize, std::vector<MyProcessInfo> & vOutput) {
	//驱动_获取进程PID列表
	std::vector<int> vPID;
	BOOL bOutListCompleted;
	BOOL b = pDriver->GetProcessPidList(vPID, FALSE, bOutListCompleted);
	printf("调用驱动 GetProcessPidList 返回值:%d\n", b);
	if (b == FALSE) {
		return FALSE;
	}
	//打印进程列表信息
	for (int pid : vPID) {
		uint64_t hProcess = pDriver->OpenProcess(pid);
		if (!hProcess) { continue; }

		MyProcessInfo pInfo = { 0 };
		pInfo.pid = pid;
		if (bGetPhyMemorySize) {
			uint64_t outRss = 0;
			pDriver->GetProcessRSS(hProcess, outRss);
			pInfo.total_rss = outRss;
		}
		char cmdline[200] = { 0 };
		pDriver->GetProcessCmdline(hProcess, cmdline, sizeof(cmdline));
		pInfo.cmdline = cmdline;

		pDriver->CloseHandle(hProcess);

		vOutput.push_back(pInfo);
	}
	return TRUE;
}


HANDLE CApi::CreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID) {

	if (dwFlags & TH32CS_SNAPPROCESS) {
		//printf("TH32CS_SNAPPROCESS\n");
		//获取进程列表
		CeProcessList * pCeProcessList = new CeProcessList();

		//获取进程列表
		GetProcessListInfo(&m_Driver, FALSE, pCeProcessList->vProcessList);


		pCeProcessList->readIter = pCeProcessList->vProcessList.begin();
		return CPortHelper::CreateHandleFromPointer((uint64_t)pCeProcessList, htTHSProcess);
	} else if (dwFlags & TH32CS_SNAPMODULE) {
		//printf("TH32CS_SNAPMODULE\n");
		HANDLE hm = CPortHelper::FindHandleByPID(th32ProcessID);
		if (!hm) {
			//如果没有打开此进程，就不允许获取此进程的模块列表
			return 0;
		}

		CeOpenProcess *pCeOpenProcess = (CeOpenProcess*)CPortHelper::GetPointerFromHandle(hm);

		//取出驱动进程句柄
		uint64_t u64DriverProcessHandle = pCeOpenProcess->u64DriverProcessHandle;


		//驱动_获取进程内存块列表（显示全部内存）
		std::vector<DRIVER_REGION_INFO> vMaps;
		BOOL bOutListCompleted;
		BOOL b = m_Driver.VirtualQueryExFull(u64DriverProcessHandle, FALSE, vMaps, bOutListCompleted);
		printf("调用驱动 VirtualQueryExFull(显示全部内存) 返回值:%d\n", b);

		if (!vMaps.size()) {
			printf("VirtualQueryExFull 失败\n");
			fflush(stdout);
			return 0;
		}

		CeModuleList * pCeModuleList = new CeModuleList();

		//显示进程内存块地址列表
		for (DRIVER_REGION_INFO rinfo : vMaps) {
			if (rinfo.protection == PAGE_NOACCESS) {
				//此地址不可访问
				continue;
			} else if (rinfo.type == MEM_MAPPED) {
				continue;
			} else if (rinfo.name[0] == '\x00' {
				continue;
			} else if (strcmp(rinfo.name, "[heap]") == 0 {
				continue;
			}
			if (strcmp(rinfo.name, "[vdso]") != 0)  //ceOpenProcessorary patch as to not rename vdso, because it is treated differently by the ce symbol loader
			{
				for (int i = 0; rinfo.name[i]; i++) //strip square brackets from the name (conflicts with pointer notations)
				{
					if ((rinfo.name[i] == '[') || (rinfo.name[i] == ']')) {
						rinfo.name[i] = '_';
					}
				}
			}

			int isExist = 0;
			for (auto iter = pCeModuleList->vModuleList.begin(); iter != pCeModuleList->vModuleList.end(); iter++) {
				if (iter->moduleName == std::string(rinfo.name)) {
					isExist = 1;
					ModuleListEntry newReplace = *iter;
					newReplace.moduleSize += rinfo.size;
					iter = pCeModuleList->vModuleList.insert(iter, newReplace);
					iter++;
					if (iter != pCeModuleList->vModuleList.end()) {
						pCeModuleList->vModuleList.erase(iter);

						break;
					}
				}
			}
			if (isExist) {
				continue;
			}

			uint32_t magic = 0;
			BOOL b = m_Driver.ReadProcessMemory(u64DriverProcessHandle, rinfo.baseaddress, &magic, 4, NULL, FALSE);
			if (b == FALSE) {
				//printf("%s is unreadable(%llx)\n", modulepath, start);
				continue; //unreadable
			}
			if (magic != 0x464c457f) //  7f 45 4c 46
			{
				//printf("%s is not an ELF(%llx).  tempbuf=%s\n", modulepath, start, tempbuf);
				continue; //not an ELF
			}

			ModuleListEntry newModInfo;
			newModInfo.baseAddress = rinfo.baseaddress;
			newModInfo.moduleSize = rinfo.size;
			newModInfo.moduleName = rinfo.name;
			//printf("%s\n", newModInfo.moduleName.c_str());

			pCeModuleList->vModuleList.push_back(newModInfo);

			//printf("+++Start:%llx,Size:%lld,Protection:%d,Type:%d,Name:%s\n", rinfo.baseaddress, rinfo.size, rinfo.protection, rinfo.type, rinfo.name);
		}

		pCeModuleList->readIter = pCeModuleList->vModuleList.begin();
		return CPortHelper::CreateHandleFromPointer((uint64_t)pCeModuleList, htTHSModule);
	}


	return 0;
}


BOOL CApi::Process32First(HANDLE hSnapshot, ProcessListEntry & processentry) {
	//Get a processentry from the processlist snapshot. fill the given processentry with the data.

   // printf("Process32First\n");
	if (CPortHelper::GetHandleType(hSnapshot) == htTHSProcess) {
		CeProcessList *pCeProcessList = (CeProcessList*)CPortHelper::GetPointerFromHandle(hSnapshot);
		pCeProcessList->readIter = pCeProcessList->vProcessList.begin();
		if (pCeProcessList->readIter != pCeProcessList->vProcessList.end()) {
			processentry.PID = pCeProcessList->readIter->pid;
			processentry.ProcessName = pCeProcessList->readIter->cmdline;

			return TRUE;
		}
	}
	return FALSE;
}


BOOL CApi::Process32Next(HANDLE hSnapshot, ProcessListEntry &processentry) {
	//get the current iterator of the list and increase it. If the max has been reached, return false
   // printf("Process32Next\n");

	if (CPortHelper::GetHandleType(hSnapshot) == htTHSProcess) {
		CeProcessList *pCeProcessList = (CeProcessList*)CPortHelper::GetPointerFromHandle(hSnapshot);
		pCeProcessList->readIter++;
		if (pCeProcessList->readIter != pCeProcessList->vProcessList.end()) {
			processentry.PID = pCeProcessList->readIter->pid;
			processentry.ProcessName = pCeProcessList->readIter->cmdline;
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CApi::Module32First(HANDLE hSnapshot, ModuleListEntry & moduleentry) {
	if (CPortHelper::GetHandleType(hSnapshot) == htTHSModule) {
		CeModuleList *pCeModuleList = (CeModuleList*)CPortHelper::GetPointerFromHandle(hSnapshot);

		pCeModuleList->readIter = pCeModuleList->vModuleList.begin();
		if (pCeModuleList->readIter != pCeModuleList->vModuleList.end()) {
			moduleentry.baseAddress = pCeModuleList->readIter->baseAddress;
			moduleentry.moduleSize = pCeModuleList->readIter->moduleSize;
			moduleentry.moduleName = pCeModuleList->readIter->moduleName;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CApi::Module32Next(HANDLE hSnapshot, ModuleListEntry & moduleentry) {
	//get the current iterator of the list and increase it. If the max has been reached, return false
	printf("Module32First/Next(%d)\n", hSnapshot);
	if (CPortHelper::GetHandleType(hSnapshot) == htTHSModule) {
		CeModuleList *pCeModuleList = (CeModuleList*)CPortHelper::GetPointerFromHandle(hSnapshot);
		pCeModuleList->readIter++;
		if (pCeModuleList->readIter != pCeModuleList->vModuleList.end()) {
			moduleentry.baseAddress = pCeModuleList->readIter->baseAddress;
			moduleentry.moduleSize = pCeModuleList->readIter->moduleSize;
			moduleentry.moduleName = pCeModuleList->readIter->moduleName;
			return TRUE;
		}
	}
	return FALSE;
}



HANDLE CApi::OpenProcess(DWORD pid) {
	//check if this process has already been opened
	HANDLE hm = CPortHelper::FindHandleByPID(pid);
	if (hm) {
		return hm;
	}
	//still here, so not opened yet

	//驱动_打开进程
	uint64_t u64DriverProcessHandle = m_Driver.OpenProcess(pid);
	if (u64DriverProcessHandle == 0) {
		return 0;
	}
	CeOpenProcess *pCeOpenProcess = new CeOpenProcess();
	pCeOpenProcess->pid = pid;
	pCeOpenProcess->u64DriverProcessHandle = u64DriverProcessHandle;
	pCeOpenProcess->nLastGetMapsTime = 0;
	return CPortHelper::CreateHandleFromPointer((uint64_t)pCeOpenProcess, htProcesHandle);
}


void CApi::CloseHandle(HANDLE h) {

	int i;
	handleType ht = CPortHelper::GetHandleType(h);
	uint64_t pl = CPortHelper::GetPointerFromHandle(h);

	printf("CloseHandle %d %" PRIu64 "\n", h, pl);

	if (ht == htTHSModule) {
		auto pCeModuleList = (CeModuleList*)pl;
		delete pCeModuleList;
	} else if (ht == htTHSProcess) {
		auto pProcessList = (CeProcessList*)pl;
		delete pProcessList;
	} else if (ht == htProcesHandle) {
		auto pOpenProcess = (CeOpenProcess*)pl;

		m_Driver.CloseHandle(pOpenProcess->u64DriverProcessHandle);
		delete pOpenProcess;
	}
	//else
	//{
	//	if (ht == htNativeThreadHandle)
	//	{
	//		uint64_t *th = GetPointerFromHandle(h);
	//		printf("Closing thread handle\n");

	//		free(th);
	//		RemoveHandle(h);
	//	}
	//	else
	//	{
	//		RemoveHandle(h); //no idea what it is...
	//	}

	//}
	CPortHelper::RemoveHandle(h);
}



int CApi::VirtualQueryExFull(HANDLE hProcess, uint32_t flags, std::vector<RegionInfo> & vRinfo)
/*
 * creates a full list of the maps file (less seeking)
 */
{
	printf("VirtualQueryExFull: %d \n", hProcess);

	if (CPortHelper::GetHandleType(hProcess) != htProcesHandle) {
		printf("VirtualQueryExFull handle Error: %d \n", hProcess);
		return 0;
	}
	CeOpenProcess *pCeOpenProcess = (CeOpenProcess*)CPortHelper::GetPointerFromHandle(hProcess);

	//取出驱动进程句柄
	uint64_t u64DriverProcessHandle = pCeOpenProcess->u64DriverProcessHandle;

	//int pagedonly = flags & VQE_PAGEDONLY;
	//int dirtyonly = flags & VQE_DIRTYONLY;
	int noshared = flags & VQE_NOSHARED;

	vRinfo.clear();

	//驱动_获取进程内存块列表（只显示在物理内存中的内存）
	std::lock_guard<std::mutex> mtxLock(pCeOpenProcess->mtxLockLastMaps);
	pCeOpenProcess->vLastMaps.clear();
	BOOL bOutListCompleted;
	m_Driver.VirtualQueryExFull(u64DriverProcessHandle, TRUE, pCeOpenProcess->vLastMaps, bOutListCompleted);
	printf("m_Driver.VirtualQueryExFull(showPhy) :%zu\n", pCeOpenProcess->vLastMaps.size());
	fflush(stdout);


	if (!pCeOpenProcess->vLastMaps.size()) {
		pCeOpenProcess->nLastGetMapsTime = 0;

		printf("m_Driver.VirtualQueryExFull(showPhy)  failed.\n");
		fflush(stdout);

		return 0;
	} else {
		//记录当前系统运行毫秒
		struct timespec times = { 0, 0 };
		clock_gettime(CLOCK_MONOTONIC, &times);
		pCeOpenProcess->nLastGetMapsTime = times.tv_sec * 1000 + times.tv_nsec / 1000000;

	}

	//显示进程内存块地址列表
	for (DRIVER_REGION_INFO rinfo : pCeOpenProcess->vLastMaps) {
		if (rinfo.protection == PAGE_NOACCESS) {
			//此地址不可访问
			continue;
		} else if (rinfo.type == MEM_MAPPED) //some checks to see if it passed
		{
			if (noshared) {
				continue;
			}
		}

		RegionInfo newInfo = { 0 };
		newInfo.baseaddress = rinfo.baseaddress;
		newInfo.size = rinfo.size;
		newInfo.protection = rinfo.protection;
		newInfo.type = rinfo.type;
		vRinfo.push_back(newInfo);
		//printf("+++Start:%llx,Size:%lld,Protection:%d,Type:%d,Name:%s\n", rinfo.baseaddress, rinfo.size, rinfo.protection, rinfo.type, rinfo.name);
	}
	return 1;

}

int CApi::VirtualQueryEx(HANDLE hProcess, uint64_t lpAddress, RegionInfo & rinfo, std::string & memName) {
	/*
	 * Alternate method: read pagemaps and look up the pfn in /proc/kpageflags (needs to 2 files open and random seeks through both files, so not sure if slow or painfully slow...)
	 */

	 //VirtualQueryEx stub port. Not a real port, and returns true if successful and false on error
	int found = 0;

	//printf("VirtualQueryEx %d (%p)\n", hProcess, lpAddress);


	if (CPortHelper::GetHandleType(hProcess) != htProcesHandle) {
		return 0;
	}

	CeOpenProcess *pCeOpenProcess = (CeOpenProcess*)CPortHelper::GetPointerFromHandle(hProcess);

	//取出驱动进程句柄
	uint64_t u64DriverProcessHandle = pCeOpenProcess->u64DriverProcessHandle;



	//驱动_获取进程内存块列表（只显示在物理内存中的内存）
	std::lock_guard<std::mutex> mtxLock(pCeOpenProcess->mtxLockLastMaps);

	//记录当前系统运行毫秒
	struct timespec times = { 0, 0 };
	clock_gettime(CLOCK_MONOTONIC, &times);
	auto nowTime = times.tv_sec * 1000 + times.tv_nsec / 1000000;
	if ((nowTime - pCeOpenProcess->nLastGetMapsTime) > 1000 * 60) //每60秒获取一次内存列表
	{
		//上次的列表超时了，重新获取一份新的
		pCeOpenProcess->vLastMaps.clear();
		BOOL bOutListCompleted;
		BOOL b = m_Driver.VirtualQueryExFull(u64DriverProcessHandle, TRUE, pCeOpenProcess->vLastMaps, bOutListCompleted);
		printf("调用驱动 VirtualQueryExFull(只显示在物理内存中的内存) 返回值:%d\n", b);
		fflush(stdout);

		if (!pCeOpenProcess->vLastMaps.size()) {
			pCeOpenProcess->nLastGetMapsTime = 0;
			printf("VirtualQueryExFull 失败\n");
			fflush(stdout);
			return 0;
		} else {
			//记录这次列表的获取时间
			memset(&times, 0, sizeof(times));
			clock_gettime(CLOCK_MONOTONIC, &times);
			pCeOpenProcess->nLastGetMapsTime = times.tv_sec * 1000 + times.tv_nsec / 1000000;
		}

	}

	rinfo.protection = 0;
	rinfo.baseaddress = (uint64_t)lpAddress & ~0xfff;
	lpAddress = (uint64_t)(rinfo.baseaddress);

	//显示进程内存块地址列表
	for (DRIVER_REGION_INFO r : pCeOpenProcess->vLastMaps) {
		uint64_t stop = r.baseaddress + r.size;
		if (stop > lpAddress) //we passed it
		{
			found = 1;

			if (lpAddress >= r.baseaddress) {
				//it's inside the region, so useable

				rinfo.protection = r.protection;
				rinfo.type = r.type;
				rinfo.size = stop - rinfo.baseaddress;
			} else {
				rinfo.size = r.baseaddress - rinfo.baseaddress;
				rinfo.protection = PAGE_NOACCESS;
				rinfo.type = 0;
			}
			memName = r.name;

			//printf("+++Start:%llx,Size:%lld %lld,Protection:%d,Type:%d\n", rinfo.baseaddress, rinfo.size, r.size, rinfo.protection, rinfo.type);
			break;
		}
	}

	return found;
}




int CApi::ReadProcessMemory(HANDLE hProcess, void *lpAddress, void *buffer, int size) {
	//idea in case this is too slow. always read a full page and keep the last 16 accessed pages.
	//only on cache miss, or if the cache is older than 1000 milliseconds fetch the page.
	//keep in mind that this routine can get called by multiple threads at the same time


	//todo: Try process_vm_readv

	  //printf("ReadProcessMemory(%d, %p, %p, %d)\n", (int)hProcess, lpAddress, buffer, size);

	//printf("ReadProcessMemory\n");


	size_t bread = 0;

	if (CPortHelper::GetHandleType(hProcess) != htProcesHandle) {
		return 0;
	}
	CeOpenProcess *pCeOpenProcess = (CeOpenProcess*)CPortHelper::GetPointerFromHandle(hProcess);

	//取出驱动进程句柄
	uint64_t u64DriverProcessHandle = pCeOpenProcess->u64DriverProcessHandle;

	//valid handle
	//驱动_读取进程内存
	m_Driver.ReadProcessMemory(u64DriverProcessHandle, (uint64_t)lpAddress, buffer, size, &bread, FALSE);
	return (int)bread;
}


int CApi::WriteProcessMemory(HANDLE hProcess, void *lpAddress, void *buffer, int size) {
	size_t written = 0;
	//printf("WriteProcessMemory(%d, %p, %p, %d\n", hProcess, lpAddress, buffer, size);


	if (CPortHelper::GetHandleType(hProcess) != htProcesHandle) {
		return 0;
	}

	CeOpenProcess *pCeOpenProcess = (CeOpenProcess*)CPortHelper::GetPointerFromHandle(hProcess);

	//取出驱动进程句柄
	uint64_t u64DriverProcessHandle = pCeOpenProcess->u64DriverProcessHandle;


	//valid handle
	//驱动_读取进程内存
	m_Driver.WriteProcessMemory(u64DriverProcessHandle, (uint64_t)lpAddress, buffer, size, &written, FALSE);

	return (int)written;
}

