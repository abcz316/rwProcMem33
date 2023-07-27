//
// Created by abcz316 on 2020/2/26.
//

#ifndef MEMSEARCHER_MAPREGIONHELPER_H
#define MEMSEARCHER_MAPREGIONHELPER_H
#include "../testKo/MemoryReaderWriter37.h"
#include "MemSearchKit/MemSearchKitUmbrella.h"
#include "MapRegionType.h"

enum RangeType {
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

//获取进程的指定内存范围列表
static BOOL GetMemRegion(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, RangeType type, BOOL showPhy, std::vector<DRIVER_REGION_INFO> & vOutput) {
	//驱动_获取进程内存块地址列表
	std::vector<DRIVER_REGION_INFO> vMapsList;
	BOOL bOutListCompleted;
	IReadWriteProxy->VirtualQueryExFull(hProcess, showPhy, vMapsList, bOutListCompleted);
	if (vMapsList.empty()) {
		//无内存
		return FALSE;
	}

	for (auto &rinfo : vMapsList) {
		bool vaild = false;
		switch (type) {
			case RangeType::X:
				vaild = is_r0xp(&rinfo);
				break;
			case RangeType::R0_0:
				vaild = is_r0_0(&rinfo);
				break;
			case RangeType::RW_0:
				vaild = is_rw_0(&rinfo);
				break;
			case RangeType::ALL:
				vaild = true;
				break;
			default:
				if (!is_rw00(&rinfo)) { continue; }
				break;
		}

		if (vaild || type == RangeType::B_BAD && is_B(&rinfo)
			|| type == RangeType::C_ALLOC && strstr(rinfo.name, "[anon:libc_malloc]")
			|| type == RangeType::C_BSS && strstr(rinfo.name, "[anon:.bss]")
			|| type == RangeType::C_DATA && strstr(rinfo.name, "/data/app/")
			|| type == RangeType::C_HEAP && is_Ch(&rinfo)
			|| type == RangeType::JAVA_HEAP && is_Jh(&rinfo)
			|| type == RangeType::A_ANONMYOUS && is_A(&rinfo)
			|| type == RangeType::CODE_SYSTEM && strstr(rinfo.name, "/system")
			|| type == RangeType::STACK && is_S(&rinfo)
			|| type == RangeType::ASHMEM && strstr(rinfo.name, "/dev/ashmem/") && !strstr(rinfo.name, "dalvik")) {
			vOutput.push_back(rinfo);
		}
	}

	return TRUE;
}

//获取进程模块执行内存起始地址
static BOOL GetMemModuleExecStartAddr(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, const std::string & moduleName,
	DRIVER_REGION_INFO & out) {
	//驱动_获取进程内存块地址列表
	std::vector<DRIVER_REGION_INFO> vMapsList;
	BOOL bOutListCompleted;
	IReadWriteProxy->VirtualQueryExFull(hProcess, TRUE, vMapsList, bOutListCompleted);
	if (vMapsList.size() == 0) {
		//无内存
		return FALSE;
	}
	const char* targetModuleName = moduleName.c_str();

	auto it = std::find_if(vMapsList.begin(), vMapsList.end(), [&](const DRIVER_REGION_INFO& rinfo){
		return is_r0xp(&rinfo) && strstr(rinfo.name, targetModuleName);
	});
	if (it != vMapsList.end()) {
		out = *it;
		return TRUE;
	}
	return FALSE;
}

//获取进程模块执行区域内存范围列表
static BOOL GetMemModuleExecAreaSection(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, const std::string & moduleName,
	std::vector<DRIVER_REGION_INFO> & vOut) {
	//驱动_获取进程内存块地址列表
	std::vector<DRIVER_REGION_INFO> vMapsList;
	BOOL bOutListCompleted;
	IReadWriteProxy->VirtualQueryExFull(hProcess, TRUE, vMapsList, bOutListCompleted);
	if (vMapsList.size() == 0) {
		//无内存
		return FALSE;
	}
	const char* targetModuleName = moduleName.c_str();

	std::copy_if(vMapsList.begin(), vMapsList.end(), std::back_inserter(vOut), 
		[&](const DRIVER_REGION_INFO& rinfo) {
			return is_r0xp(&rinfo) && strstr(rinfo.name, targetModuleName);
		}
	);

	return TRUE;
}

//获取进程模块数据区域内存范围列表
static BOOL GetMemModuleDataAreaSection(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, const std::string & moduleName,
	std::vector<DRIVER_REGION_INFO> & vOut) {
	//1.获取进程内存块地址列表
	std::vector<DRIVER_REGION_INFO> vMapsList;
	BOOL bOutListCompleted;
	IReadWriteProxy->VirtualQueryExFull(hProcess, TRUE, vMapsList, bOutListCompleted);
	if (vMapsList.size() == 0) {
		//无内存
		return FALSE;
	}

	const char* targetModuleName = moduleName.c_str();

	bool bReadCheckDataSec = false;
	bool bLastIsDataSec = false;

	for (size_t x = 0; x < vMapsList.size(); x++) {
		auto & rinfo = vMapsList[x];

		if (bLastIsDataSec || bReadCheckDataSec) {
			bool bCurDataSection = is_rw_0(&rinfo) || is_r__0(&rinfo) || is_0w_0(&rinfo);

			if (bReadCheckDataSec && bCurDataSection) {
				bLastIsDataSec = true;
			}
			bReadCheckDataSec = false;

			if (bCurDataSection) {
				//数据段中
				vOut.push_back(rinfo);
				continue;
			}
			//数据中断了
			bLastIsDataSec = false;
		}

		if (is_r0xp(&rinfo) && strstr(rinfo.name, targetModuleName)) {
			bReadCheckDataSec = true;
		}
	}
	return !vOut.empty();
}


//获取进程模块的内存地址范围
static BOOL GetMemModuleRangeAddr(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, const std::string & moduleName,
	uint64_t & outModuleStartAddr, uint64_t & outModuleEndAddr) {

	//获取so的内存范围
	const char * targetModuleName = moduleName.c_str();
	std::vector<DRIVER_REGION_INFO> vExecAreaMemSec;
	std::vector<DRIVER_REGION_INFO> vDataAreaMemSec;
	GetMemModuleExecAreaSection(IReadWriteProxy, hProcess, targetModuleName, vExecAreaMemSec);
	GetMemModuleDataAreaSection(IReadWriteProxy, hProcess, targetModuleName, vDataAreaMemSec);
	if (!vExecAreaMemSec.size() || !vDataAreaMemSec.size()) {
		//printf("GetMemModuleExecAreaSection || GetMemModuleDataAreaSection失败");
		return FALSE;
	}
	if (vExecAreaMemSec[0].baseaddress >= vDataAreaMemSec[0].baseaddress) {
		//printf("发生未知异常：内存模块的数据区在执行代码区的上方");
		return FALSE;
	}
	outModuleStartAddr = vExecAreaMemSec[0].baseaddress;
	outModuleEndAddr = max(vExecAreaMemSec[vExecAreaMemSec.size() - 1].baseaddress,
		vDataAreaMemSec[vDataAreaMemSec.size() - 1].baseaddress);
	return TRUE;
}


#endif //MEMSEARCHER_MAPREGIONHELPER_H
