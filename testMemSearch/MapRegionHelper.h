//
// Created by abcz316 on 2020/2/26.
//

#ifndef MEMSEARCHER_MAPREGIONHELPER_H
#define MEMSEARCHER_MAPREGIONHELPER_H
#include "../testKo/MemoryReaderWriter37.h"
#include "MemSearchHelper.h"
#include "MapRegionType.h"

enum RangeType
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
	X,      //执行命令内存 r0xp
	R0_0,      //可读非执行内存 r0_0
	RW_0       //可读可写非执行内存 rw_0
};

//获取进程的内存块区域
static BOOL GetMemRegion(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, RangeType type, BOOL showPhy, std::vector<MEM_SECTION_INFO> & vOutput)
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
	vOutput.clear();
	for (DRIVER_REGION_INFO rinfo : vMapsList) {
		int vaild = 0;
		if (type == RangeType::X)
		{
			if (is_r0xp(&rinfo)) { vaild = 1; }
		}
		else if (type == RangeType::R0_0)
		{
			if (is_r0_0(&rinfo)) { vaild = 1; }
		}
		else if (type == RangeType::RW_0)
		{
			if (is_rw_0(&rinfo)) { vaild = 1; }
		}
		else if (type == RangeType::ALL) { vaild = 1; }

		else if (!is_rw00(&rinfo)) { continue; }

		else if (type == RangeType::B_BAD)
		{
			if (is_B(&rinfo)) { vaild = 1; }
		}
		else if (type == RangeType::C_ALLOC)
		{
			if (strstr(rinfo.name, "[anon:libc_malloc]")) { vaild = 1; }
		}
		else if (type == RangeType::C_BSS)
		{
			if (strstr(rinfo.name, "[anon:.bss]")) { vaild = 1; }
		}
		else if (type == RangeType::C_DATA)
		{
			if (strstr(rinfo.name, "/data/app/")) { vaild = 1; }
		}
		else if (type == RangeType::C_HEAP)
		{
			if (is_Ch(&rinfo)) { vaild = 1; }
		}
		else if (type == RangeType::JAVA_HEAP)
		{
			if (is_Jh(&rinfo)) { vaild = 1; }
		}
		else if (type == RangeType::A_ANONMYOUS)
		{
			if (is_A(&rinfo)) { vaild = 1; }
		}
		else if (type == RangeType::CODE_SYSTEM)
		{
			if (strstr(rinfo.name, "/system")) { vaild = 1; }
		}
		else if (type == RangeType::STACK)
		{
			if (is_S(&rinfo)) { vaild = 1; }
		}
		else if (type == RangeType::ASHMEM)
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

//获取进程模块执行内存起始地址
static BOOL GetMemModuleExecStartAddr(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, const std::string & moduleName,
	MEM_SECTION_INFO & out) {
	//驱动_获取进程内存块地址列表
	std::vector<DRIVER_REGION_INFO> vMapsList;
	BOOL bOutListCompleted;
	IReadWriteProxy->VirtualQueryExFull(hProcess, TRUE, vMapsList, bOutListCompleted);
	if (vMapsList.size() == 0) {
		//无内存
		return FALSE;
	}
	const char* targetModuleName = moduleName.c_str();
	for (DRIVER_REGION_INFO rinfo : vMapsList) {
		if (!is_r0xp(&rinfo)) {
			continue;
		}
		if (strstr(rinfo.name, targetModuleName)) {
			out.npSectionAddr = rinfo.baseaddress;
			out.nSectionSize = rinfo.size;
			break;
		}
	}
	return TRUE;
}

//获取进程模块执行区域内存地址
static BOOL GetMemModuleExecAreaSection(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, const std::string & moduleName,
	std::vector<MEM_SECTION_INFO> & vOut) {
	//驱动_获取进程内存块地址列表
	std::vector<DRIVER_REGION_INFO> vMapsList;
	BOOL bOutListCompleted;
	IReadWriteProxy->VirtualQueryExFull(hProcess, TRUE, vMapsList, bOutListCompleted);
	if (vMapsList.size() == 0) {
		//无内存
		return FALSE;
	}
	const char* targetModuleName = moduleName.c_str();
	for (DRIVER_REGION_INFO rinfo : vMapsList) {
		if (!is_r0xp(&rinfo)) {
			continue;
		}
		if (strstr(rinfo.name, targetModuleName)) {
			MEM_SECTION_INFO newSec;
			newSec.npSectionAddr = rinfo.baseaddress;
			newSec.nSectionSize = rinfo.size;
			vOut.push_back(newSec);
		}
	}
	return TRUE;
}


//获取进程模块数据区域内存地址
static BOOL GetMemModuleDataAreaSection(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, const std::string & moduleName,
	std::vector<MEM_SECTION_INFO> & vOut) {
	//1.获取进程内存块地址列表
	std::vector<DRIVER_REGION_INFO> vMapsList;
	BOOL bOutListCompleted;
	IReadWriteProxy->VirtualQueryExFull(1, TRUE, vMapsList, bOutListCompleted);
	if (vMapsList.size() == 0) {
		//无内存
		return FALSE;
	}

	std::vector<DRIVER_REGION_INFO> vLibUE4_so;
	uint64_t startExecuteAddr = 0;
	uint64_t startAddr = 0; //遍历起始位置
	uint64_t endAddr = 0; //遍历结束位置
	const char* targetModuleName = moduleName.c_str();
	for (DRIVER_REGION_INFO rinfo : vMapsList) {
		if (startExecuteAddr == 0 && strstr(rinfo.name, targetModuleName) && is_r0xp(&rinfo)) {
			startExecuteAddr = rinfo.baseaddress; //记录模块起始位置
		}
		else if (startExecuteAddr && startAddr == 0 && is_rw00(&rinfo)) {
			startAddr = rinfo.baseaddress; //设置遍历起始位置

			MEM_SECTION_INFO newSec;
			newSec.npSectionAddr = rinfo.baseaddress;
			newSec.nSectionSize = rinfo.size;
			vOut.push_back(newSec);

		}
		else if (startExecuteAddr && startAddr && is_rw00(&rinfo)) {
			endAddr = rinfo.baseaddress + rinfo.size; //设置遍历结束位置，一个so模块的区域，包括后面无名内存

			MEM_SECTION_INFO newSec;
			newSec.npSectionAddr = rinfo.baseaddress;
			newSec.nSectionSize = rinfo.size;
			vOut.push_back(newSec);
		}
		else if (startExecuteAddr && startAddr && endAddr && is_r0xp(&rinfo)) {
			break;
		}
	}
	if (!startAddr || !endAddr) {
		return FALSE;
	}
	return TRUE;
}


#endif //MEMSEARCHER_MAPREGIONHELPER_H
