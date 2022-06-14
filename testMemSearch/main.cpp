#include <cstdio>
#include <string.h> 
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <memory>
#include <dirent.h>
#include <cinttypes>
#include <mutex>
#include "MapRegionHelper.h"
#include "MemSearchKit/MemSearchKitUmbrella.h"


using namespace MemorySearchKit;

int findPID(const char *lpszCmdline, CMemoryReaderWriter *pDriver) {
	int nTargetPid = 0;

	//驱动_获取进程PID列表
	std::vector<int> vPID;
	BOOL bOutListCompleted;
	BOOL b = pDriver->GetProcessPidList(vPID, FALSE, bOutListCompleted);
	printf("调用驱动 GetProcessPidList 返回值:%d\n", b);

	//打印进程列表信息
	for (int pid : vPID) {
		//驱动_打开进程
		uint64_t hProcess = pDriver->OpenProcess(pid);
		if (!hProcess) { continue; }

		//驱动_获取进程命令行
		char cmdline[100] = { 0 };
		pDriver->GetProcessCmdline(hProcess, cmdline, sizeof(cmdline));

		//驱动_关闭进程
		pDriver->CloseHandle(hProcess);

		if (strcmp(lpszCmdline, cmdline) == 0) {
			nTargetPid = pid;
			break;
		}
	}
	return nTargetPid;
}

//演示多线程普通搜索
void normal_val_search(CMemoryReaderWriter *pRwDriver, uint64_t hProcess, size_t nWorkThreadCount) {

	//获取进程数据内存区域
	std::vector<DRIVER_REGION_INFO> vScanMemMaps;
	GetMemRegion(pRwDriver, hProcess,
		R0_0,
		TRUE/*FALSE为全部内存，TRUE为只在物理内存中的内存*/,
		vScanMemMaps);
	if (!vScanMemMaps.size()) {
		printf("无内存可搜索\n");
		//关闭进程
		pRwDriver->CloseHandle(hProcess);
		printf("调用驱动 CloseHandle:%" PRIu64 "\n", hProcess);
		fflush(stdout);
		return;
	}
	//准备要搜索的内存区域
	std::shared_ptr<MemSearchSafeWorkSecWrapper> spvWaitScanMemSec = std::make_shared<MemSearchSafeWorkSecWrapper>();
	if (!spvWaitScanMemSec) {
		return;
	}
	for (auto & item : vScanMemMaps) {
		spvWaitScanMemSec->push_back(item.baseaddress, item.size, 0, item.size);
	}

	//首次搜索
	std::vector<ADDR_RESULT_INFO> vSearchResult; //搜索结果
	{
		SearchValue<float>(
			pRwDriver,
			hProcess,
			spvWaitScanMemSec, //待搜索的内存区域
			0.33333334327f, //搜索数值
			0.0f,
			0.01, //误差范围
			SCAN_TYPE::ACCURATE_VAL, //搜索类型: 精确搜索
			nWorkThreadCount, //搜索线程数
			vSearchResult, //搜索后的结果
			4); //扫描的对齐字节数

		printf("共搜索出%zu个地址\n", vSearchResult.size());
	}
	//再次搜索
	if (vSearchResult.size()) {
		//将每个地址往后偏移20
		std::vector<ADDR_RESULT_INFO> vWaitSearchAddr; //待搜索的内存地址列表
		for (auto item : vSearchResult) {
			item.addr += 20;
			vWaitSearchAddr.push_back(item);
		}

		//再次搜索
		vSearchResult.clear();

		std::vector<ADDR_RESULT_INFO> vErrorList;
		SearchAddrNextValue<float>(
			pRwDriver,
			hProcess,
			vWaitSearchAddr, //待搜索的内存地址列表
			1.19175350666f, //搜索数值
			0.0f,
			0.01f, //误差范围
			SCAN_TYPE::ACCURATE_VAL, //搜索类型: 精确搜索
			nWorkThreadCount, //搜索线程数
			vSearchResult,
			vErrorList); //搜索后的结果
	}

	//再次搜索
	if (vSearchResult.size()) {
		//将每个地址往后偏移952
		std::vector<ADDR_RESULT_INFO> vWaitSearchAddr; //待搜索的内存地址列表
		for (auto item : vSearchResult) {
			item.addr += 952;
			vWaitSearchAddr.push_back(item);
		}

		//再次搜索
		vSearchResult.clear();
		std::vector<ADDR_RESULT_INFO> vErrorList;
		SearchAddrNextValue<int>(
			pRwDriver,
			hProcess,
			vWaitSearchAddr, //待搜索的内存地址列表
			-2147483648, //搜索数值
			0,
			0.01, //误差范围
			SCAN_TYPE::ACCURATE_VAL, //搜索类型: 精确搜索
			nWorkThreadCount, //搜索线程数
			vSearchResult, //搜索后的结果
			vErrorList);
	}

	size_t count = 0;
	for (size_t i = 0; i < vSearchResult.size(); i++) {

		ADDR_RESULT_INFO addr = vSearchResult.at(i);
		printf("addr:%p\n", (void*)addr.addr);
		count++;
		if (count > 100) {
			printf("只显示前100个地址\n");
			break;
		}

	}
	printf("共偏移搜索出%zu个地址\n", vSearchResult.size());
	if (vSearchResult.size()) {
		printf("第一个地址为:%p\n", (void*)vSearchResult.at(0).addr);
	}


//演示多线程正向遍历
void loop_search(CMemoryReaderWriter *pRwDriver, uint64_t hProcess, size_t nWorkThreadCount) {


	//获取进程内存块地址列表
	const char * targetModuleName = "libxxx.so";
	std::vector<DRIVER_REGION_INFO> vModuleList;
	GetMemModuleDataAreaSection(pRwDriver, 1, targetModuleName, vModuleList);

	DRIVER_REGION_INFO execStart;
	GetMemModuleExecStartAddr(pRwDriver, 1, targetModuleName, execStart);
	if (!execStart.baseaddress) {
		printf("GetMemModuleStartAddr失败");
		return;
	}
	uint64_t execStartAddr = execStart.baseaddress;

	//整理需要工作的内存区域
	std::shared_ptr<MemSearchSafeWorkSecWrapper> spWorkMemWrapper = std::make_shared<MemSearchSafeWorkSecWrapper>();
	for (auto & item : vModuleList) {
		spWorkMemWrapper->push_back(item.baseaddress, item.size, 0, item.size);
	}
	if (!spWorkMemWrapper->normal_block_count()) {
		printf("GetMemModuleDataAreaSection失败");
		return;
	}

	struct OffsetResultInfo {
		uint64_t offset[4] = { 0 };
	};
	std::vector<OffsetResultInfo> vResultInfo; //遍历结果

	MultiThreadExecOnCpu(nWorkThreadCount,
		[pRwDriver, hProcess, spWorkMemWrapper, execStartAddr, &vResultInfo]
		(size_t thread_id, std::atomic<bool> *pForceStopSignal)->void {

		//////////////////////////////////////////////////////////////////////////

		std::vector<OffsetResultInfo> vThreadOutput; //存放当前线程的搜索结果
		uint64_t curMemBlockStartAddr = 0;
		uint64_t curMemBlockSize = 0;
		std::shared_ptr<std::atomic<uint64_t>> spOutCurWorkOffset;
		std::shared_ptr<unsigned char> spOutMemDataBlock;

		while (!pForceStopSignal || !*pForceStopSignal) {
			if (!spOutCurWorkOffset) {
				if (!spWorkMemWrapper->get_need_work_mem_sec(curMemBlockStartAddr, curMemBlockSize, spOutCurWorkOffset, spOutMemDataBlock)) {
					break; //没任务了
				}
			}
			uint64_t curWorkOffset = spOutCurWorkOffset->fetch_add(4); //每次遍历前进4字节
			if (curWorkOffset >= curMemBlockSize) {
				spOutCurWorkOffset = nullptr;
				spOutMemDataBlock = nullptr;
				continue;
			}
			//DEBUG
			//uint64_t debugAddr = execStartAddr + 0xAD58338;
			//if (curMemBlockStartAddr <= debugAddr && (curMemBlockStartAddr + curMemBlockSize) > debugAddr) {
			//	curWorkOffset = debugAddr - curMemBlockStartAddr;
			//} else {
			//	continue;
			//}

			//////////////////////////////////////////////////////////////////////////

			//读取这个目标内存section的内存数据时直接指针取值，节省开销
			size_t firstReadLen = 8;
			if ((curMemBlockSize - curWorkOffset) < firstReadLen) {
				continue;
			}
			uint64_t addr1 = *(uint64_t*)((char*)spOutMemDataBlock.get() + curWorkOffset);
			//////////////////////////////////////////////////////////////////////////
			if (!pRwDriver->CheckMemAddrIsValid(hProcess, addr1)) {
				continue;
			}
			for (uint64_t x2 = 0; x2 < 0x200; x2 += 4) {
				uint64_t addr2 = 0;
				if (!pRwDriver->ReadProcessMemory(hProcess, addr1 + x2, &addr2, 8, NULL)) {
					continue;
				}
				if (!pRwDriver->CheckMemAddrIsValid(hProcess, addr2)) {
					continue;
				}

				for (uint64_t x3 = 0; x3 < 0x200; x3 += 4) {
					uint64_t addr3 = 0;
					if (!pRwDriver->ReadProcessMemory(1, addr2 + x3, &addr3, 8, NULL)) {
						continue;
					}
					if (!pRwDriver->CheckMemAddrIsValid(hProcess, addr3)) {
						continue;
					}

					for (uint64_t x4 = 0; x4 < 0x200; x4 += 4) {
						uint64_t addr4 = 0;
						if (!pRwDriver->ReadProcessMemory(1, addr3 + x4, &addr4, 8, NULL)) {
							continue;
						}
						if (!pRwDriver->CheckMemAddrIsValid(hProcess, addr4)) {
							continue;
						}

						//目标
						uint64_t target = 0x7AF6E21600;
						if (addr4 != target) {
							continue;
						}

						OffsetResultInfo newOffset;
						newOffset.offset[0] = curMemBlockStartAddr + curWorkOffset - execStartAddr;
						newOffset.offset[1] = x2;
						newOffset.offset[2] = x3;
						newOffset.offset[3] = x4;
						vThreadOutput.push_back(newOffset);
					}
				}
			}
		}
		//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
		for (auto & item : vThreadOutput) {
			vResultInfo.push_back(item);
		}
	});

	FILE *stream = fopen("result.txt", "w+"); //_wfopen
	if (stream) {
		for (OffsetResultInfo & item : vResultInfo) {
			/* write some data to the file */
			fprintf(stream, "[0]->%lx [1]->%lx [2]->%lx [3]->%lx\n",
				item.offset[0],
				item.offset[1],
				item.offset[2],
				item.offset[3]);
		}
		/* close the file */
		fclose(stream);
	}

}
}

//演示多线程反向遍历
void reverse_search(CMemoryReaderWriter* pRwDriver, uint64_t hProcess, size_t nWorkThreadCount) {

	//获取进程内存区域
	std::vector<DRIVER_REGION_INFO> vScanMemMaps;
	GetMemRegion(pRwDriver, hProcess,
		R0_0,
		TRUE/*FALSE为全部内存，TRUE为只在物理内存中的内存*/,
		vScanMemMaps);
	if (!vScanMemMaps.size()) {
		printf("无内存可搜索\n");
		//关闭进程
		pRwDriver->CloseHandle(hProcess);
		printf("调用驱动 CloseHandle:%" PRIu64 "\n", hProcess);
		fflush(stdout);
		return;
	}


	//准备要搜索的内存区域
	std::shared_ptr<MemSearchSafeWorkSecWrapper> spvWaitScanMemSec = std::make_shared<MemSearchSafeWorkSecWrapper>();
	if (!spvWaitScanMemSec) {
		return;
	}
	for (auto& item : vScanMemMaps) {
		spvWaitScanMemSec->push_back(item.baseaddress, item.size, 0, item.size);
	}


	//首次反向搜索
	std::vector<std::shared_ptr<baseOffsetInfo>> vBaseAddrOffsetRecord; //记录搜索到的地址偏移
	std::map<uint64_t, std::shared_ptr<std::vector<std::shared_ptr<baseOffsetInfo>>>> linkAddrOffsetHelperMap; //辅助再次反向搜索找到父节点的map
	std::vector<BATCH_BETWEEN_VAL_ADDR_RESULT<uint64_t>> vBetweenValSearchResult; //反向搜索结果
	{

		std::vector<BATCH_BETWEEN_VAL<uint64_t>> vWaitSearchBetweenVal; //待搜索的两值之间的数值数组

		//开始批量填充待搜索的两值之间的数值数组
		//for (size_t i = 0; i < 10; i++) {
		//	BATCH_BETWEEN_VAL<uint64_t> newBeteenVal;
		//	memset(&newBeteenVal, 0, sizeof(newBeteenVal));

		//	uint64_t tmpAddr = 0x401000 + i; //演示地址
		//	newBeteenVal.val1 = tmpAddr - 0x250;
		//	newBeteenVal.val2 = tmpAddr + 0x250;
		//	newBeteenVal.markContext.u64Ctx = tmpAddr;//传递保存一个标识地址
		//	vWaitSearchBetweenVal.push_back(newBeteenVal);
		//}


		//演示只反向搜索一个地址
		BATCH_BETWEEN_VAL<uint64_t> newBeteenVal;
		memset(&newBeteenVal, 0, sizeof(newBeteenVal));
		uint64_t tmpAddr = 0x401000; //演示地址
		newBeteenVal.val1 = tmpAddr - 0x250;
		newBeteenVal.val2 = tmpAddr + 0x250;
		newBeteenVal.markContext.u64Ctx = tmpAddr;//传递保存一个标识地址
		vWaitSearchBetweenVal.push_back(newBeteenVal);

		SearchBatchBetweenValue<uint64_t>(
			pRwDriver,
			1,
			spvWaitScanMemSec, //待搜索的内存区域
			vWaitSearchBetweenVal, //待搜索的两值之间的数值数组
			nWorkThreadCount, //搜索线程数
			vBetweenValSearchResult, //搜索后的结果
			4); //扫描的对齐字节数



		printf("1.共搜索出%zu个地址\n", vBetweenValSearchResult.size());

		//记录搜索到的偏移
		for (size_t i = 0; i < vBetweenValSearchResult.size(); i++) {
			auto& addrResult = vBetweenValSearchResult.at(i);

			uint64_t curAddrVal = *(uint64_t*)addrResult.addrInfo.spSaveData.get();

			std::shared_ptr<baseOffsetInfo> spBaseOffset = std::make_shared<baseOffsetInfo>();
			memset(spBaseOffset.get(), 0, sizeof(baseOffsetInfo));
			spBaseOffset->addr = addrResult.addrInfo.addr;
			spBaseOffset->offset = addrResult.originalCondition.markContext.u64Ctx - curAddrVal;
			vBaseAddrOffsetRecord.push_back(spBaseOffset);

			//辅助再次反向搜索找到父节点的vector
			std::shared_ptr<std::vector<std::shared_ptr<baseOffsetInfo>>> spvBaseInfoObj
				= std::make_shared<std::vector<std::shared_ptr<baseOffsetInfo>>>();
			spvBaseInfoObj->push_back(spBaseOffset);
			linkAddrOffsetHelperMap[spBaseOffset->addr] = spvBaseInfoObj;

		}
	}

	//再次反向搜索
	if (vBetweenValSearchResult.size()) {

		//进度恢复
		spvWaitScanMemSec->recover_normal_block_origin_progress();

		std::vector<BATCH_BETWEEN_VAL<uint64_t>> vWaitSearchBetweenVal; //待搜索的两值之间的数值数组
		for (auto& addrResult : vBetweenValSearchResult) {
			BATCH_BETWEEN_VAL<uint64_t> newBeteenVal;
			memset(&newBeteenVal, 0, sizeof(newBeteenVal));
			newBeteenVal.val1 = addrResult.addrInfo.addr - 0x250;
			newBeteenVal.val2 = addrResult.addrInfo.addr + 0x250;
			newBeteenVal.markContext.u64Ctx = addrResult.addrInfo.addr;//传递保存一个标识地址
			vWaitSearchBetweenVal.push_back(newBeteenVal);
		}
		//DEBUG
		//BATCH_BETWEEN_VAL<uint64_t> newBeteenVal;
		//memset(&newBeteenVal, 0, sizeof(newBeteenVal));
		//newBeteenVal.val1 = 0x77d55d4df0 - 0x250;
		//newBeteenVal.val2 = 0x77d55d4df0 + 0x250;
		//newBeteenVal.markContext.u64Ctx = 0x77d55d4df0;//传递保存一个标值地址
		//vWaitSearchBetweenVal.push_back(newBeteenVal);


		vBetweenValSearchResult.clear();

		SearchBatchBetweenValue<uint64_t>(
			pRwDriver,
			1,
			spvWaitScanMemSec, //待搜索的内存区域
			vWaitSearchBetweenVal, //待搜索的两值之间的数值数组
			nWorkThreadCount, //搜索线程数
			vBetweenValSearchResult, //搜索后的结果
			4); //扫描的对齐字节数

		printf("2.共搜索出%zu个地址\n", vBetweenValSearchResult.size());

		//记录搜索到的偏移
		for (size_t i = 0; i < vBetweenValSearchResult.size(); i++) {
			auto& addrResult = vBetweenValSearchResult.at(i);

			uint64_t curAddrVal = *(uint64_t*)addrResult.addrInfo.spSaveData.get();

			std::shared_ptr<baseOffsetInfo> spBaseOffset = std::make_shared<baseOffsetInfo>();
			memset(spBaseOffset.get(), 0, sizeof(baseOffsetInfo));
			spBaseOffset->addr = addrResult.addrInfo.addr;
			spBaseOffset->offset = addrResult.originalCondition.markContext.u64Ctx - curAddrVal;
			vBaseAddrOffsetRecord.push_back(spBaseOffset);

			//辅助再次反向搜索找到父节点的vector
			if (linkAddrOffsetHelperMap.find(spBaseOffset->addr) == linkAddrOffsetHelperMap.end()) {
				std::shared_ptr<std::vector<std::shared_ptr<baseOffsetInfo>>> spvBaseInfoObj
					= std::make_shared<std::vector<std::shared_ptr<baseOffsetInfo>>>();
				spvBaseInfoObj->push_back(spBaseOffset);
				linkAddrOffsetHelperMap[spBaseOffset->addr] = spvBaseInfoObj;
			}
			else {
				linkAddrOffsetHelperMap[spBaseOffset->addr]->push_back(spBaseOffset);
			}

			//将反向搜索到的地址偏移串联起来
			auto spvLastBaseInfoObj = linkAddrOffsetHelperMap[addrResult.originalCondition.markContext.u64Ctx];
			for (auto spLastAddrNode : *spvLastBaseInfoObj) {
				spLastAddrNode->vwpNextNode.push_back(spBaseOffset);
				spBaseOffset->vwpLastNode.push_back(spLastAddrNode);
			}


		}
	}

	//再次反向搜索
	if (vBetweenValSearchResult.size()) {

		//进度恢复
		spvWaitScanMemSec->recover_normal_block_origin_progress();

		std::vector<BATCH_BETWEEN_VAL<uint64_t>> vWaitSearchBetweenVal; //待搜索的两值之间的数值数组
		for (auto& addrResult : vBetweenValSearchResult) {
			BATCH_BETWEEN_VAL<uint64_t> newBeteenVal;
			memset(&newBeteenVal, 0, sizeof(newBeteenVal));
			newBeteenVal.val1 = addrResult.addrInfo.addr - 0x250;
			newBeteenVal.val2 = addrResult.addrInfo.addr + 0x250;
			newBeteenVal.markContext.u64Ctx = addrResult.addrInfo.addr;//传递保存一个标识地址
			vWaitSearchBetweenVal.push_back(newBeteenVal);

		}
		////DEBUG
		//BATCH_BETWEEN_VAL<uint64_t> newBeteenVal;
		//memset(&newBeteenVal, 0, sizeof(newBeteenVal));
		//newBeteenVal.val1 = 0x780e7f56e0 - 0x250;
		//newBeteenVal.val2 = 0x780e7f56e0 + 0x250;
		//newBeteenVal.markContext.u64Ctx = 0x780e7f56e0;//传递保存一个标值地址
		//vWaitSearchBetweenVal.push_back(newBeteenVal);
		vBetweenValSearchResult.clear();

		SearchBatchBetweenValue<uint64_t>(
			pRwDriver,
			1,
			spvWaitScanMemSec, //待搜索的内存区域
			vWaitSearchBetweenVal, //待搜索的两值之间的数值数组
			nWorkThreadCount, //搜索线程数
			vBetweenValSearchResult, //搜索后的结果
			4); //扫描的对齐字节数

		printf("3.共搜索出%zu个地址\n", vBetweenValSearchResult.size());

		//记录搜索到的偏移
		for (size_t i = 0; i < vBetweenValSearchResult.size(); i++) {
			auto& addrResult = vBetweenValSearchResult.at(i);

			uint64_t curAddrVal = *(uint64_t*)addrResult.addrInfo.spSaveData.get();

			std::shared_ptr<baseOffsetInfo> spBaseOffset = std::make_shared<baseOffsetInfo>();
			memset(spBaseOffset.get(), 0, sizeof(baseOffsetInfo));
			spBaseOffset->addr = addrResult.addrInfo.addr;
			spBaseOffset->offset = addrResult.originalCondition.markContext.u64Ctx - curAddrVal;
			vBaseAddrOffsetRecord.push_back(spBaseOffset);

			//辅助再次反向搜索找到父节点的vector
			if (linkAddrOffsetHelperMap.find(spBaseOffset->addr) == linkAddrOffsetHelperMap.end()) {
				std::shared_ptr<std::vector<std::shared_ptr<baseOffsetInfo>>> spvBaseInfoObj
					= std::make_shared<std::vector<std::shared_ptr<baseOffsetInfo>>>();
				spvBaseInfoObj->push_back(spBaseOffset);
				linkAddrOffsetHelperMap[spBaseOffset->addr] = spvBaseInfoObj;
			}
			else {
				linkAddrOffsetHelperMap[spBaseOffset->addr]->push_back(spBaseOffset);
			}

			//将反向搜索到的地址偏移串联起来
			auto spvLastBaseInfoObj = linkAddrOffsetHelperMap[addrResult.originalCondition.markContext.u64Ctx];
			for (auto spLastAddrNode : *spvLastBaseInfoObj) {
				spLastAddrNode->vwpNextNode.push_back(spBaseOffset);
				spBaseOffset->vwpLastNode.push_back(spLastAddrNode);
			}


		}
	}



	//反向搜索结束归纳
	if (vBetweenValSearchResult.size()) {

		//打印整个链路
		std::vector<singleOffsetLinkPath> vTotalShow; //显示的总路径文本
		for (auto iter = vBaseAddrOffsetRecord.begin(); iter != vBaseAddrOffsetRecord.end(); iter++) {
			auto spBaseAddrOffsetInfo = *iter;
			//last节点为空，next节点不为空才是头部节点
			if (spBaseAddrOffsetInfo->vwpLastNode.size() == 0 && spBaseAddrOffsetInfo->vwpNextNode.size()) {
				AddrOffsetLinkMapToVector(spBaseAddrOffsetInfo, [&vTotalShow](
					const singleOffsetLinkPath& newSinglePath, size_t deepIndex/*start from zero*/)->void {
						//TODO：这里可以限制自己想要的层级
						if (deepIndex != 3 - 1) {
							return;//层级不够深
						}
						//TODO：在这里可以对基址进行一些过滤，比如只保留某个.so范围内的addr
						//uint64_t finalBaseAddr = std::get<0>(newSinglePath[newSinglePath.size() - 1]);
						//static uint64_t moduleStartAddr = 0, moduleEndAddr = 0;
						//if (moduleStartAddr == 0 || moduleEndAddr == 0) {
						//	GetMemModuleRangeAddr(pRwDriver, hProcess, "libxxx.so", moduleStartAddr, moduleEndAddr);
						//	assert(moduleStartAddr && moduleEndAddr);
						//}
						//if (finalBaseAddr < moduleStartAddr || finalBaseAddr > moduleEndAddr) {
						//	return; //超出了某个.so的范围
						//}
						vTotalShow.push_back(newSinglePath);
					});
			}
		}
		//去重
		std::set<singleOffsetLinkPath>s(vTotalShow.begin(), vTotalShow.end());
		vTotalShow.assign(s.begin(), s.end());

		//显示出来
		std::stringstream sstrTotalShow;
		for (auto& item : vTotalShow) {

			//把偏移转成文本
			for (auto& child : item) {
				//十进制转十六进制
				sstrTotalShow << "+0x" << std::hex << std::get<0>(child);
				ssize_t offset = std::get<1>(child);

				if (offset < 0) {
					sstrTotalShow << "-0x" << std::hex << -offset;
				}
				else {
					sstrTotalShow << "+0x" << std::hex << offset;
				}
				sstrTotalShow << "]";
			}
			sstrTotalShow << "\n";
		}
		FILE* stream = fopen("result.txt", "w+"); //_wfopen
		if (stream) {
			/* write some data to the file */
			fprintf(stream, "%s\n", sstrTotalShow.str().c_str()); //%S

			/* close the file */
			fclose(stream);
		}

	}



}


int main(int argc, char *argv[]) {
	printf(
		"======================================================\n"
		"本驱动名称: Linux ARM64 硬件读写进程内存驱动37\n"
		"本驱动接口列表：\n"
		"\t1.	 驱动_设置驱动设备接口文件允许同时被使用的最大值: SetMaxDevFileOpen\n"
		"\t2.	 驱动_隐藏驱动（卸载驱动需重启机器）: HideKernelModule\n"
		"\t3.	 驱动_打开进程: OpenProcess\n"
		"\t4.	 驱动_读取进程内存: ReadProcessMemory\n"
		"\t5.	 驱动_写入进程内存: WriteProcessMemory\n"
		"\t6.	 驱动_关闭进程: CloseHandle\n"
		"\t7.	 驱动_获取进程内存块列表: VirtualQueryExFull（可选：显示全部内存、只显示在物理内存中的内存）\n"
		"\t8.	 驱动_获取进程PID列表: GetProcessPidList\n"
		"\t9.	 驱动_获取进程权限等级: GetProcessGroup\n"
		"\t10.驱动_提升进程权限到Root: SetProcessRoot\n"
		"\t11.驱动_获取进程占用物理内存大小: GetProcessRSS\n"
		"\t12.驱动_获取进程命令行: GetProcessCmdline\n"
		"\t以上所有功能不注入、不附加进程，不打开进程任何文件，所有操作均为内核操作\n"
		"======================================================\n"
	);


	CMemoryReaderWriter rwDriver;


	//驱动默认文件名
	std::string devFileName = RWPROCMEM_FILE_NODE;
	if (argc > 1) {
		//如果用户自定义输入驱动名
		devFileName = argv[1];
	}
	printf("Connecting rwDriver:%s\n", devFileName.c_str());


	//连接驱动
	int err = 0;
	if (!rwDriver.ConnectDriver(devFileName.c_str(), err)) {
		printf("Connect rwDriver failed. error:%d\n", err);
		fflush(stdout);
		return 0;
	}



	//获取目标进程PID
	const char *name = "com.miui.calculator";
	pid_t pid = findPID(name, &rwDriver);
	if (pid == 0) {
		printf("找不到进程\n");
		fflush(stdout);
		return 0;
	}
	printf("目标进程PID:%d\n", pid);
	//打开进程
	uint64_t hProcess = rwDriver.OpenProcess(pid);
	printf("调用驱动 OpenProcess 返回值:%" PRIu64 "\n", hProcess);
	if (!hProcess) {
		printf("调用驱动 OpenProcess 失败\n");
		fflush(stdout);
		return 0;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	size_t nWorkThreadCount = std::thread::hardware_concurrency() - 1;
	//1.演示多线程普通搜索
	normal_val_search(&rwDriver, hProcess, nWorkThreadCount);
	//2.演示多线程正向遍历
	loop_search(&rwDriver, hProcess, nWorkThreadCount);
	//3.演示多线程反向遍历
	reverse_search(&rwDriver, hProcess, nWorkThreadCount);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//关闭进程
	rwDriver.CloseHandle(hProcess);
	printf("调用驱动 CloseHandle:%" PRIu64 "\n", hProcess);
	fflush(stdout);
	return 0;
}