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
	if (vMapsList.size() == 0) {
		//无内存
		return FALSE;
	}

	//存放即将要搜索的内存区域
	vOutput.clear();
	for (DRIVER_REGION_INFO rinfo : vMapsList) {
		bool vaild = false;
		if (type == RangeType::X) {
			if (is_r0xp(&rinfo)) { vaild = true; }
		} else if (type == RangeType::R0_0) {
			if (is_r0_0(&rinfo)) { vaild = true; }
		} else if (type == RangeType::RW_0) {
			if (is_rw_0(&rinfo)) { vaild = true; }
		} else if (type == RangeType::ALL) { vaild = true; }

		else if (!is_rw00(&rinfo)) { continue; }

		else if (type == RangeType::B_BAD) {
			if (is_B(&rinfo)) { vaild = true; }
		} else if (type == RangeType::C_ALLOC) {
			if (strstr(rinfo.name, "[anon:libc_malloc]")) { vaild = true; }
		} else if (type == RangeType::C_BSS) {
			if (strstr(rinfo.name, "[anon:.bss]")) { vaild = true; }
		} else if (type == RangeType::C_DATA) {
			if (strstr(rinfo.name, "/data/app/")) { vaild = true; }
		} else if (type == RangeType::C_HEAP) {
			if (is_Ch(&rinfo)) { vaild = true; }
		} else if (type == RangeType::JAVA_HEAP) {
			if (is_Jh(&rinfo)) { vaild = true; }
		} else if (type == RangeType::A_ANONMYOUS) {
			if (is_A(&rinfo)) { vaild = true; }
		} else if (type == RangeType::CODE_SYSTEM) {
			if (strstr(rinfo.name, "/system")) { vaild = true; }
		} else if (type == RangeType::STACK) {
			if (is_S(&rinfo)) { vaild = true; }
		} else if (type == RangeType::ASHMEM) {
			if (strstr(rinfo.name, "/dev/ashmem/") && !strstr(rinfo.name, "dalvik")) { vaild = true; }
		}

		if (vaild) {
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
	for (DRIVER_REGION_INFO rinfo : vMapsList) {
		if (!is_r0xp(&rinfo)) {
			continue;
		}
		if (strstr(rinfo.name, targetModuleName)) {
			out = rinfo;
			break;
		}
	}
	return TRUE;
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
	for (DRIVER_REGION_INFO rinfo : vMapsList) {
		if (!is_r0xp(&rinfo)) {
			continue;
		}
		if (strstr(rinfo.name, targetModuleName)) {
			vOut.push_back(rinfo);
		}
	}
	return TRUE;
}

//获取进程模块数据区域内存范围列表
static BOOL GetMemModuleDataAreaSection(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, const std::string & moduleName,
	bool dataSectionCanRead, bool dataSectionCanWrite, 
	std::vector<DRIVER_REGION_INFO> & vOut) {
	//1.获取进程内存块地址列表
	std::vector<DRIVER_REGION_INFO> vMapsList;
	BOOL bOutListCompleted;
	IReadWriteProxy->VirtualQueryExFull(1, TRUE, vMapsList, bOutListCompleted);
	if (vMapsList.size() == 0) {
		//无内存
		return FALSE;
	}
	if (!dataSectionCanRead && !dataSectionCanWrite) {
		return FALSE;
	}

	std::vector<DRIVER_REGION_INFO> vLibUE4_so;
	const char* targetModuleName = moduleName.c_str();

	bool bLastIsDataSec = false;
	for (size_t x = 0; x < vMapsList.size(); x++) {
		auto & rinfo = vMapsList.at(x);

		if (bLastIsDataSec) {
			bool bCurDataSection = false;
			if (dataSectionCanRead && dataSectionCanWrite) {
				if (is_rw_0(&rinfo)) {
					bCurDataSection = true;
				}
			} else if(dataSectionCanRead) {
				if (is_r__0(&rinfo)) {
					bCurDataSection = true;
				}
			} else if (dataSectionCanWrite) {
				if (is_0w_0(&rinfo)) {
					bCurDataSection = true;
				}
			}

			if (bCurDataSection) {
				//数据段中
				vOut.push_back(rinfo);
				continue;
			}
			//数据中断了
			bLastIsDataSec = false;
		}

		if (strstr(rinfo.name, targetModuleName) && is_r0xp(&rinfo)) {
			size_t y = x + 1;
			if (y < vMapsList.size()) {
				auto & rinfoNext = vMapsList.at(y);
				if (is_rw00(&rinfoNext)) {
					//数据起始位置
					bLastIsDataSec = true;
					vOut.push_back(rinfoNext);
				}
			}
		}
	}
	return !!vOut.size();
}

//获取进程模块的内存地址范围
static BOOL GetMemModuleRangeAddr(IMemReaderWriterProxy *IReadWriteProxy, uint64_t hProcess, const std::string & moduleName,
	uint64_t & outModuleStartAddr, uint64_t & outModuleEndAddr) {

	//获取so的内存范围
	const char * targetModuleName = moduleName.c_str();
	std::vector<DRIVER_REGION_INFO> vExecAreaMemSec;
	std::vector<DRIVER_REGION_INFO> vDataAreaMemSec;
	GetMemModuleExecAreaSection(IReadWriteProxy, hProcess, targetModuleName, vExecAreaMemSec);
	GetMemModuleDataAreaSection(IReadWriteProxy, hProcess, targetModuleName, true, true, vDataAreaMemSec);
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
