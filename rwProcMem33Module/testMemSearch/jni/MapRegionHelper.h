//
// Created by abcz316 on 2020/2/26.
//

#ifndef MEMSEARCHER_MAPREGIONHELPER_H
#define MEMSEARCHER_MAPREGIONHELPER_H
#include "../../testKo/jni/MemoryReaderWriter39.h"
#include "MemSearchKit/MemSearchKitUmbrella.h"
#include "ProcMapsFileReader.h"

enum RegionType {
    REGION_C_HEAP = 1,
    REGION_JAVA_HEAP,
    REGION_C_ALLOC,
    REGION_C_DATA,
    REGION_C_BSS,
    REGION_ANONYMOUS,
    REGION_STACK,
    REGION_CODE_APP,
    REGION_CODE_SYS,
    REGION_JAVA,
    REGION_BAD,
    /*REGION_PPSSPP,*/
    REGION_ASHMEM,
    REGION_VIDEO,
    REGION_OTHER,
    REGION_ALL,
	REGION_X,      //执行命令内存 r0xp
	REGION_R0_0,  //可读非执行内存 r0_0
	REGION_RW_0  //可读可写非执行内存 rw_0
};

struct MemRegionItem {
	DRIVER_REGION_INFO baseInfo;
	std::string belongSoPath;
};

// 获取进程的指定内存范围列表
static BOOL GetMemRegions(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, RegionType type, std::vector<MemRegionItem> & vOutput) {
	std::vector<DRIVER_REGION_INFO> vMapsList;
	IReadWriteProxy->VirtualQueryExFull(hProcess, FALSE, vMapsList);
	if (vMapsList.empty()) {
		return FALSE;
	}
	std::string lastSoPath;
	for (auto &rinfo : vMapsList) {
		if((strstr(rinfo.name, "/data/app/") || strstr(rinfo.name, "/data/data/")) && strstr(rinfo.name, ".so")) {
			lastSoPath = rinfo.name;
		}
		//printf("rinfo.name : %s\n", rinfo.name);
		bool vaild = false;
		if(type == REGION_ALL) {
			vaild = true;
		} else if(type == REGION_X) {
			vaild = is_r0xp(&rinfo);
		} else if(type == REGION_R0_0) {
			vaild = is_r0_0(&rinfo);
		} else if(type == REGION_RW_0) {
			vaild = is_rw_0(&rinfo);
		} else if(strlen(rinfo.name) == 0) {
			vaild = type == REGION_ANONYMOUS;
		} else if(strstr(rinfo.name, "/dev/asheme/")) {
			vaild = type == REGION_ASHMEM;
		} else if(strstr(rinfo.name, "/system/fonts/")) {
			vaild = type == REGION_BAD;
		} else if(strstr(rinfo.name, ".so") && is_r0xp(&rinfo)) {
			vaild = (strstr(rinfo.name, "/data/app/") || strstr(rinfo.name, "/data/data/")) ? type == REGION_CODE_APP : type == REGION_CODE_SYS;
		} else if(strstr(rinfo.name, "[anon:libc_malloc") ||  strstr(rinfo.name, "[anon:scudo:")) {
			vaild = type == REGION_C_ALLOC;
		} else if(strstr(rinfo.name, "[anon:.bss")) {
			vaild = type == REGION_C_BSS;
		} else if(strstr(rinfo.name, "/data/app/") && strstr(rinfo.name, ".so")) {
			vaild = type == REGION_C_DATA;
		} else if(strstr(rinfo.name, "[heap]")) {
			vaild = type == REGION_C_HEAP;
		} else if(strstr(rinfo.name, "dalvik-allocation") || strstr(rinfo.name, "dalvik-main") || strstr(rinfo.name, "dalvik-large") 
			|| strstr(rinfo.name, "dalvik-free")) {
			vaild = type == REGION_JAVA_HEAP;
		} else if(strstr(rinfo.name, "dalvik-CompilerMetadata") || strstr(rinfo.name, "dalvik-LinearAlloc") 
			|| strstr(rinfo.name, "dalvik-indirect") || strstr(rinfo.name, "dalvik-rosalloc") || strstr(rinfo.name, "dalvik-card") 
			|| strstr(rinfo.name, "dalvik-mark") ||  (strstr(rinfo.name, "dalvik-")  && strstr(rinfo.name, "space") )) {
			vaild = type == REGION_JAVA;
		} else if(strstr(rinfo.name, "[stack")) {
			vaild = type == REGION_STACK;
		} else if(strstr(rinfo.name, "/dev/kgsl-3d0")) {
			vaild = type == REGION_VIDEO;
		} else {
			vaild = type == REGION_OTHER;
		}
		if(vaild) {
			MemRegionItem item;
			memcpy(&item.baseInfo, &rinfo, sizeof(rinfo));
			item.belongSoPath = lastSoPath;
			vOutput.push_back(item);
		}
	}
	return TRUE;
}

// 获取内存模块第一个起始地址
static BOOL GetModuleFirstAddr(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, const std::string & moduleName, uint64_t & firstAddr) {
	std::vector<DRIVER_REGION_INFO> vMapsList;
	IReadWriteProxy->VirtualQueryExFull(hProcess, FALSE, vMapsList);
	if (vMapsList.size() == 0) {
		//无内存
		return FALSE;
	}
	const char* targetModuleName = moduleName.c_str();
	for(auto & rinfo : vMapsList) {
		if(strstr(rinfo.name, targetModuleName)) {
			firstAddr = rinfo.baseaddress;
			return TRUE;
		}
	}
	return FALSE;
}

// 获取内存模块执行区域内存范围列表
static BOOL GetModuleExecAreaSection(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, const std::string & moduleName, std::vector<DRIVER_REGION_INFO> & vOut) {
	std::vector<DRIVER_REGION_INFO> vMapsList;
	IReadWriteProxy->VirtualQueryExFull(hProcess, FALSE, vMapsList);
	if (vMapsList.size() == 0) {
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

// 获取内存模块数据区域内存范围列表
static BOOL GetModuleDataAreaSection(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, const std::string & moduleName, std::vector<DRIVER_REGION_INFO> & vOut) {
	std::vector<DRIVER_REGION_INFO> vMapsList;
	IReadWriteProxy->VirtualQueryExFull(hProcess, FALSE, vMapsList);
	if (vMapsList.size() == 0) {
		return FALSE;
	}

	const char* targetModuleName = moduleName.c_str();

	bool bReadCheckDataSec = false;
	bool bLastIsDataSec = false;

	for (size_t x = 0; x < vMapsList.size(); x++) {
		auto & rinfo = vMapsList[x];

		if (bLastIsDataSec || bReadCheckDataSec) {
			bool bCurDataSection = is_rw_0(&rinfo) || is_r__0(&rinfo) || is_0w_0(&rinfo); // 减少了一个 is_rw_0 的调用

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


// 获取内存模块的内存地址范围
static BOOL GetModuleRangeAddr(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, const std::string & moduleName,
	uint64_t & outModuleStartAddr, uint64_t & outModuleEndAddr) {

	//获取so的内存范围
	const char * targetModuleName = moduleName.c_str();
	uint64_t firstAddr = 0;
	std::vector<DRIVER_REGION_INFO> vExecAreaMemSec;
	std::vector<DRIVER_REGION_INFO> vDataAreaMemSec;
	GetModuleFirstAddr(IReadWriteProxy, hProcess, targetModuleName, firstAddr);
	GetModuleExecAreaSection(IReadWriteProxy, hProcess, targetModuleName, vExecAreaMemSec);
	GetModuleDataAreaSection(IReadWriteProxy, hProcess, targetModuleName, vDataAreaMemSec);
	if (!vExecAreaMemSec.size() || !vDataAreaMemSec.size()) {
		//printf("GetModuleExecAreaSection || GetModuleDataAreaSection失败");
		return FALSE;
	}
	outModuleStartAddr = min(vExecAreaMemSec[0].baseaddress, vDataAreaMemSec[0].baseaddress);
	outModuleStartAddr = min(outModuleStartAddr, firstAddr);
	outModuleEndAddr = max(vExecAreaMemSec[vExecAreaMemSec.size() - 1].baseaddress,
		vDataAreaMemSec[vDataAreaMemSec.size() - 1].baseaddress);
	return TRUE;
}


#endif //MEMSEARCHER_MAPREGIONHELPER_H
