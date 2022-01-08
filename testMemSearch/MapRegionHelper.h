//
// Created by abcz316 on 2020/2/26.
//

#ifndef MEMSEARCHER_MAPREGIONHELPER_H
#define MEMSEARCHER_MAPREGIONHELPER_H
#include "../testKo/MemoryReaderWriter37.h"
#include "MemSearchHelper.h"
#include "MapRegionType.h"

enum Range
{
	ALL,        //所有内存
	B_BAD,      //B内存
	C_ALLOC,    //Ca内存
	C_BSS,      //Cb内存
	C_DATA,     //Cd内存
	C_HEAP,     //Ch内存
	JAVA_HEAP,  //Jh内存
	A_ANONMYOUS,//A内存
	CODE_SYSTEM,//Xs内存 r-xp
	//CODE_APP /data/ r-xp

	STACK,      //S内存
	ASHMEM,      //As内存
	X      //执行命令内存 r0xp
};

//获取进程的内存块区域
static BOOL GetMemRegion(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, int type, BOOL showPhy, std::vector<MEM_SECTION_INFO> & vOutput)
{
	//驱动_获取进程内存块地址列表
	std::vector<DRIVER_REGION_INFO> vMapsList;
	BOOL bOutListCompleted;
	IReadWriteProxy->VirtualQueryExFull(hProcess, showPhy, vMapsList, bOutListCompleted);
	if (vMapsList.size() == 0)
	{
		//无内存
		return FALSE;
	}


	//存放即将要搜索的内存区域
	int vaild = 0;
	vOutput.clear();
	for (DRIVER_REGION_INFO rinfo : vMapsList)
	{
		if (type == Range::X)
		{
			if (is_r0xp(&rinfo)) { vaild = 1; }
		}
		else if (type == Range::ALL) { vaild = 1; }

		else if (!is_rw00(&rinfo)) { continue; }

		else if (type == Range::B_BAD)
		{
			if (is_B(&rinfo)) { vaild = 1; }
		}
		else if (type == Range::C_ALLOC)
		{
			if (strstr(rinfo.name, "[anon:libc_malloc]")) { vaild = 1; }
		}
		else if (type == Range::C_BSS)
		{
			if (strstr(rinfo.name, "[anon:.bss]")) { vaild = 1; }
		}
		else if (type == Range::C_DATA)
		{
			if (strstr(rinfo.name, "/data/app/")) { vaild = 1; }
		}
		else if (type == Range::C_HEAP)
		{
			if (is_Ch(&rinfo)) { vaild = 1; }
		}
		else if (type == Range::JAVA_HEAP)
		{
			if (is_Jh(&rinfo)) { vaild = 1; }
		}
		else if (type == Range::A_ANONMYOUS)
		{
			if (is_A(&rinfo)) { vaild = 1; }
		}
		else if (type == Range::CODE_SYSTEM)
		{
			if (strstr(rinfo.name, "/system")) { vaild = 1; }
		}
		else if (type == Range::STACK)
		{
			if (is_S(&rinfo)) { vaild = 1; }
		}
		else if (type == Range::ASHMEM)
		{
			if (strstr(rinfo.name, "/dev/ashmem/") && !strstr(rinfo.name, "dalvik")) { vaild = 1; }
		}

		if (vaild == 1)
		{
			MEM_SECTION_INFO newMemScan;
			newMemScan.npSectionAddr = rinfo.baseaddress;
			newMemScan.nSectionSize = rinfo.size;
			vOutput.push_back(newMemScan);
		}
	}

	return TRUE;
}

//获取进程模块地址
static BOOL GetMemModuleStartAddr(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, const std::string & moduleName,
	MEM_SECTION_INFO & out) {
	//驱动_获取进程内存块地址列表
	std::vector<DRIVER_REGION_INFO> vMapsList;
	BOOL bOutListCompleted;
	IReadWriteProxy->VirtualQueryExFull(hProcess, TRUE, vMapsList, bOutListCompleted);
	if (vMapsList.size() == 0) {
		//无内存
		return FALSE;
	}

	for (DRIVER_REGION_INFO rinfo : vMapsList) {
		if (!is_r0xp(&rinfo)) {
			continue;
		}
		if (std::string(rinfo.name).find(moduleName) != -1) {
			out.npSectionAddr = rinfo.baseaddress;
			out.nSectionSize = rinfo.size;
			return TRUE;
		}
	}
	return FALSE;
}
static BOOL GetMemModuleAddr(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, const std::string & moduleName,
	std::vector<DRIVER_REGION_INFO> &vOut) {
	//驱动_获取进程内存块地址列表
	std::vector<DRIVER_REGION_INFO> vMapsList;
	BOOL bOutListCompleted;
	IReadWriteProxy->VirtualQueryExFull(hProcess, TRUE, vMapsList, bOutListCompleted);
	if (vMapsList.size() == 0) {
		//无内存
		return FALSE;
	}

	for (DRIVER_REGION_INFO rinfo : vMapsList) {
		if (std::string(rinfo.name).find(moduleName) != -1) {
			vOut.push_back(rinfo);
		}
	}
	return TRUE;
}

#endif //MEMSEARCHER_MAPREGIONHELPER_H
