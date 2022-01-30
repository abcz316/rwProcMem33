#include <cstdio>
#include <string.h> 
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <memory>
#include <dirent.h>
#include <cinttypes>
#include "MapRegionHelper.h"
#include "MemSearchHelper.h"
#include "ReverseSearchAddrLinkMapHelper.h"



int findPID(const char *lpszCmdline, CMemoryReaderWriter *pDriver)
{
	int nTargetPid = 0;

	//驱动_获取进程PID列表
	std::vector<int> vPID;
	BOOL bOutListCompleted;
	BOOL b = pDriver->GetProcessPidList(vPID, FALSE, bOutListCompleted);
	printf("调用驱动 GetProcessPidList 返回值:%d\n", b);

	//打印进程列表信息
	for (int pid : vPID)
	{
		//驱动_打开进程
		uint64_t hProcess = pDriver->OpenProcess(pid);
		if (!hProcess) { continue; }

		//驱动_获取进程命令行
		char cmdline[100] = { 0 };
		pDriver->GetProcessCmdline(hProcess, cmdline, sizeof(cmdline));

		//驱动_关闭进程
		pDriver->CloseHandle(hProcess);

		if (strcmp(lpszCmdline, cmdline) == 0)
		{
			nTargetPid = pid;
			break;
		}
	}
	return nTargetPid;
}

//演示多线程正向搜索
void normal_val_search(CMemoryReaderWriter *pRwDriver, uint64_t hProcess, SafeVector<ADDR_RESULT_INFO> & vSearchResult) {

	//获取进程ANONMYOUS内存区域
	std::vector<MEM_SECTION_INFO> vScanMemMaps;
	GetMemRegion(pRwDriver, hProcess,
		R0_0,
		TRUE/*FALSE为全部内存，TRUE为只在物理内存中的内存*/,
		vScanMemMaps);
	if (!vScanMemMaps.size())
	{
		printf("无内存可搜索\n");
		//关闭进程
		pRwDriver->CloseHandle(hProcess);
		printf("调用驱动 CloseHandle:%" PRIu64 "\n", hProcess);
		fflush(stdout);
		return;
	}

	//首次搜索
	//SafeVector<ADDR_RESULT_INFO> vSearchResult; //搜索结果
	{
		SafeVector<MEM_SECTION_INFO> vScanMemMapsList(vScanMemMaps); //搜索结果
		SafeVector<MEM_SECTION_INFO> vSearcMemSecError;


		//非阻塞模式
		//std::thread td1(SearchMemoryThread<float>,.....);
		//td1.detach();


		//阻塞模式
		SearchMemoryThread<float>(
			pRwDriver,
			hProcess,
			vScanMemMapsList, //待搜索的内存区域
			0.33333334327f, //搜索数值
			0.0f,
			0.01, //误差范围
			SCAN_TYPE::ACCURATE_VAL, //搜索类型: 精确搜索
			std::thread::hardware_concurrency() - 1, //搜索线程数
			4, //快速扫描的对齐位数，CE默认为4
			vSearchResult, //搜索后的结果
			vSearcMemSecError);


		printf("共搜索出%zu个地址\n", vSearchResult.size());
	}
	//再次搜索
	while (vSearchResult.size()) {
		//将每个地址往后偏移20
		SafeVector<ADDR_RESULT_INFO> vWaitSearchAddr; //待搜索的内存地址列表

		ADDR_RESULT_INFO addr;
		while (vSearchResult.get_val(addr))
		{
			addr.addr += 20;
			vWaitSearchAddr.push_back(addr);
		}

		//再次搜索
		vSearchResult.clear();

		SafeVector<ADDR_RESULT_INFO> vSearcAddrError;
		SearchNextMemoryThread<float>(
			pRwDriver,
			hProcess,
			vWaitSearchAddr, //待搜索的内存地址列表
			1.19175350666f, //搜索数值
			0.0f,
			0.01f, //误差范围
			SCAN_TYPE::ACCURATE_VAL, //搜索类型: 精确搜索
			std::thread::hardware_concurrency() - 1, //搜索线程数
			vSearchResult,  //搜索后的结果
			vSearcAddrError);
		break;
	}

	//再次搜索
	while (vSearchResult.size()) {
		//将每个地址往后偏移952
		SafeVector<ADDR_RESULT_INFO> vWaitSearchAddr; //待搜索的内存地址列表

		ADDR_RESULT_INFO addr;
		while (vSearchResult.get_val(addr))
		{
			addr.addr += 952;
			vWaitSearchAddr.push_back(addr);
		}

		//再次搜索
		vSearchResult.clear();

		SafeVector<ADDR_RESULT_INFO> vSearcAddrError;
		SearchNextMemoryThread<int>(
			pRwDriver,
			hProcess,
			vWaitSearchAddr, //待搜索的内存地址列表
			-2147483648, //搜索数值
			0,
			0.01, //误差范围
			SCAN_TYPE::ACCURATE_VAL, //搜索类型: 精确搜索
			std::thread::hardware_concurrency() - 1, //搜索线程数
			vSearchResult,  //搜索后的结果
			vSearcAddrError);
		break;
	}

	size_t count = 0;
	for (size_t i = 0; i < vSearchResult.size(); i++) {

		ADDR_RESULT_INFO addr = vSearchResult.at(i);
		printf("addr:%p\n", (void*)addr.addr);
		count++;
		if (count > 100)
		{
			printf("只显示前100个地址\n");
			break;
		}

	}
	printf("共偏移搜索出%zu个地址\n", vSearchResult.size());
	if (vSearchResult.size())
	{
		printf("第一个地址为:%p\n", (void*)vSearchResult.at(0).addr);
	}

}

//演示多线程反向搜索
void reverse_search(CMemoryReaderWriter *pRwDriver, uint64_t hProcess, SafeVector<ADDR_RESULT_INFO> & vSearchResult) {

	//获取进程ANONMYOUS内存区域
	std::vector<MEM_SECTION_INFO> vScanMemMaps;
	GetMemRegion(pRwDriver, hProcess,
		R0_0,
		TRUE/*FALSE为全部内存，TRUE为只在物理内存中的内存*/,
		vScanMemMaps);
	if (!vScanMemMaps.size())
	{
		printf("无内存可搜索\n");
		//关闭进程
		pRwDriver->CloseHandle(hProcess);
		printf("调用驱动 CloseHandle:%" PRIu64 "\n", hProcess);
		fflush(stdout);
		return;
	}


	//反向搜索
	std::vector<std::shared_ptr<baseOffsetInfo>> vBaseAddrOffsetRecord; //记录搜索到的地址偏移
	std::map<uint64_t, std::shared_ptr<std::vector<std::shared_ptr<baseOffsetInfo>>>> linkAddrOffsetHelperMap; //辅助再次反向搜索找到父节点的map

	SafeVector<BATCH_BETWEEN_VAL_ADDR_RESULT<uint64_t>> vBetweenValSearchResult; //反向搜索结果
	while (vSearchResult.size()) {

		SafeVector<MEM_SECTION_INFO> vScanMemMapsList(vScanMemMaps);

		std::vector<BATCH_BETWEEN_VAL<uint64_t>> vWaitSearchBetweenVal; //待搜索的两值之间的数值数组

		//开始批量填充待搜索的两值之间的数值数组
		//std::vector<ADDR_RESULT_INFO> vLastAddrResult;
		//vSearchResult.copy_vals(vLastAddrResult);
		//for (auto & addrResult : vLastAddrResult) {
		//	BATCH_BETWEEN_VAL<uint64_t> newBeteenVal;
		//	memset(&newBeteenVal, 0, sizeof(newBeteenVal));
		//	newBeteenVal.val1 = addrResult.addr - 0x300;
		//	newBeteenVal.val2 = addrResult.addr + 0x300;
		//	newBeteenVal.markContext.u64Ctx = addrResult.addr;//传递保存一个标值地址
		//	vWaitSearchBetweenVal.push_back(newBeteenVal);
		//}

		//演示只反向搜索一个地址
		uint64_t tmpAddr = vSearchResult.at(0).addr;
		BATCH_BETWEEN_VAL<uint64_t> newBeteenVal;
		memset(&newBeteenVal, 0, sizeof(newBeteenVal));
		newBeteenVal.val1 = tmpAddr - 0x300;
		newBeteenVal.val2 = tmpAddr + 0x300;
		newBeteenVal.markContext.u64Ctx = tmpAddr;//传递保存一个标值地址
		vWaitSearchBetweenVal.push_back(newBeteenVal);


		SafeVector<MEM_SECTION_INFO> vSearcMemSecError; //错误内存段
		//阻塞模式
		SearchMemoryBatchBetweenValThread<uint64_t>(
			pRwDriver,
			1,
			vScanMemMapsList, //待搜索的内存区域
			vWaitSearchBetweenVal, //待搜索的两值之间的数值数组
			std::thread::hardware_concurrency() - 1, //搜索线程数
			4, //快速扫描的对齐位数，CE默认为4
			vBetweenValSearchResult, //搜索后的结果
			vSearcMemSecError);


		printf("1.共搜索出%zu个地址\n", vBetweenValSearchResult.size());

		//记录搜索到的偏移
		for (size_t i = 0; i < vBetweenValSearchResult.size(); i++) {
			auto & addrResult = vBetweenValSearchResult.at(i);

			uint64_t curAddrVal = *(uint64_t*)addrResult.addrInfo.spSaveData.get();

			std::shared_ptr<baseOffsetInfo> spBaseOffset = std::make_shared<baseOffsetInfo>();
			memset(spBaseOffset.get(), 0, sizeof(baseOffsetInfo));
			spBaseOffset->addr = addrResult.addrInfo.addr;
			spBaseOffset->offset = addrResult.originalCondition.markContext.u64Ctx - curAddrVal;
			vBaseAddrOffsetRecord.push_back(spBaseOffset);

			//辅助再次反向搜索找到父节点的vector
			std::shared_ptr<std::vector<std::shared_ptr<baseOffsetInfo>>> spvBaseInfoObj = std::make_shared<std::vector<std::shared_ptr<baseOffsetInfo>>>();
			spvBaseInfoObj->push_back(spBaseOffset);
			linkAddrOffsetHelperMap[spBaseOffset->addr] = spvBaseInfoObj;

		}
		break;
	}

	//再次反向搜索
	while (vBetweenValSearchResult.size()) {

		SafeVector<MEM_SECTION_INFO> vScanMemMapsList(vScanMemMaps);

		std::vector<BATCH_BETWEEN_VAL<uint64_t>> vWaitSearchBetweenVal; //待搜索的两值之间的数值数组


		std::vector<BATCH_BETWEEN_VAL_ADDR_RESULT<uint64_t>> vLastAddrResult;
		vBetweenValSearchResult.copy_vals(vLastAddrResult);
		for (auto & addrResult : vLastAddrResult) {
			BATCH_BETWEEN_VAL<uint64_t> newBeteenVal;
			memset(&newBeteenVal, 0, sizeof(newBeteenVal));
			newBeteenVal.val1 = addrResult.addrInfo.addr - 0x300;
			newBeteenVal.val2 = addrResult.addrInfo.addr + 0x300;
			newBeteenVal.markContext.u64Ctx = addrResult.addrInfo.addr;//传递保存一个标值地址
			vWaitSearchBetweenVal.push_back(newBeteenVal);

		}

		vBetweenValSearchResult.clear();
		SafeVector<MEM_SECTION_INFO> vSearcMemSecError;

		//阻塞模式
		SearchMemoryBatchBetweenValThread<uint64_t>(
			pRwDriver,
			1,
			vScanMemMapsList, //待搜索的内存区域
			vWaitSearchBetweenVal, //待搜索的两值之间的数值数组
			std::thread::hardware_concurrency() - 1, //搜索线程数
			4, //快速扫描的对齐位数，CE默认为4
			vBetweenValSearchResult, //搜索后的结果
			vSearcMemSecError);

		printf("2.共搜索出%zu个地址\n", vBetweenValSearchResult.size());

		//记录搜索到的偏移
		for (size_t i = 0; i < vBetweenValSearchResult.size(); i++) {
			auto & addrResult = vBetweenValSearchResult.at(i);

			uint64_t curAddrVal = *(uint64_t*)addrResult.addrInfo.spSaveData.get();

			std::shared_ptr<baseOffsetInfo> spBaseOffset = std::make_shared<baseOffsetInfo>();
			memset(spBaseOffset.get(), 0, sizeof(baseOffsetInfo));
			spBaseOffset->addr = addrResult.addrInfo.addr;
			spBaseOffset->offset = addrResult.originalCondition.markContext.u64Ctx - curAddrVal;
			vBaseAddrOffsetRecord.push_back(spBaseOffset);

			//辅助再次反向搜索找到父节点的vector
			if (linkAddrOffsetHelperMap.find(spBaseOffset->addr) == linkAddrOffsetHelperMap.end()) {
				std::shared_ptr<std::vector<std::shared_ptr<baseOffsetInfo>>> spvBaseInfoObj = std::make_shared<std::vector<std::shared_ptr<baseOffsetInfo>>>();
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


		break;
	}
	//再次反向搜索
	while (vBetweenValSearchResult.size()) {

		SafeVector<MEM_SECTION_INFO> vScanMemMapsList(vScanMemMaps);

		std::vector<BATCH_BETWEEN_VAL<uint64_t>> vWaitSearchBetweenVal; //待搜索的两值之间的数值数组


		std::vector<BATCH_BETWEEN_VAL_ADDR_RESULT<uint64_t>> vTempAddrResult;
		vBetweenValSearchResult.copy_vals(vTempAddrResult);
		for (auto & addrResult : vTempAddrResult) {
			BATCH_BETWEEN_VAL<uint64_t> newBeteenVal;
			memset(&newBeteenVal, 0, sizeof(newBeteenVal));
			newBeteenVal.val1 = addrResult.addrInfo.addr;
			newBeteenVal.val2 = addrResult.addrInfo.addr;
			newBeteenVal.markContext.u64Ctx = addrResult.addrInfo.addr;//传递保存一个标值地址
			vWaitSearchBetweenVal.push_back(newBeteenVal);

		}

		vBetweenValSearchResult.clear();
		SafeVector<MEM_SECTION_INFO> vSearcMemSecError;

		//阻塞模式
		SearchMemoryBatchBetweenValThread<uint64_t>(
			pRwDriver,
			1,
			vScanMemMapsList, //待搜索的内存区域
			vWaitSearchBetweenVal, //待搜索的两值之间的数值数组
			std::thread::hardware_concurrency() - 1, //搜索线程数
			4, //快速扫描的对齐位数，CE默认为4
			vBetweenValSearchResult, //搜索后的结果
			vSearcMemSecError);

		printf("3.共搜索出%zu个地址\n", vBetweenValSearchResult.size());

		//记录搜索到的偏移
		for (size_t i = 0; i < vBetweenValSearchResult.size(); i++) {
			auto & addrResult = vBetweenValSearchResult.at(i);

			uint64_t curAddrVal = *(uint64_t*)addrResult.addrInfo.spSaveData.get();


			std::shared_ptr<baseOffsetInfo> spBaseOffset = std::make_shared<baseOffsetInfo>();
			memset(spBaseOffset.get(), 0, sizeof(baseOffsetInfo));
			spBaseOffset->addr = addrResult.addrInfo.addr;
			spBaseOffset->offset = addrResult.originalCondition.markContext.u64Ctx - curAddrVal;
			vBaseAddrOffsetRecord.push_back(spBaseOffset);

			//辅助再次反向搜索找到父节点的vector
			if (linkAddrOffsetHelperMap.find(spBaseOffset->addr) == linkAddrOffsetHelperMap.end()) {

				std::shared_ptr<std::vector<std::shared_ptr<baseOffsetInfo>>> spvBaseInfoObj = std::make_shared<std::vector<std::shared_ptr<baseOffsetInfo>>>();
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

		//打印整个链路
		std::map<std::string, unsigned char> totalShowMap; //显示的文本总路径，为了不重复
		for (auto iter = vBaseAddrOffsetRecord.begin(); iter != vBaseAddrOffsetRecord.end(); iter++) {
			auto spBaseAddrOffsetInfo = *iter;
			//last节点为空，next节点不为空才是头部节点
			if (spBaseAddrOffsetInfo->vwpLastNode.size() == 0 && spBaseAddrOffsetInfo->vwpNextNode.size()) {
				PrintfAddrOffsetLinkMap(spBaseAddrOffsetInfo, [&totalShowMap](const std::string & newSinglePath, size_t deepIndex/*start from zero*/)->void {
					if (deepIndex != 5 - 1) {
						return;//层级不够深
					}
					if (totalShowMap.find(newSinglePath) == totalShowMap.end()) {
						totalShowMap[newSinglePath] = 0;
					}
				});
			}
		}
		std::string strTotalShow;
		for (auto iter = totalShowMap.begin(); iter != totalShowMap.end(); iter++) {
			strTotalShow += iter->first;
			strTotalShow += "\n";
		}
		FILE *stream = fopen("result.txt", "w+"); //_wfopen
		if (stream) {
			/* write some data to the file */
			fprintf(stream, "%s\n", strTotalShow.c_str()); //%S

			/* close the file */
			fclose(stream);
		}
		break;
	}
}

//演示多线程遍历搜索
void loop_search(CMemoryReaderWriter *pRwDriver, uint64_t hProcess, SafeVector<ADDR_RESULT_INFO> & vSearchResult) {

	std::vector<DRIVER_REGION_INFO> vLibxxx_so;
	if (!GetMemModuleAddr(
		dynamic_cast<IMemReaderWriterProxy*>(pRwDriver),
		1, "libxxx.so", vLibxxx_so)) {
		printf("GetMemModuleStartAddr失败");
		return;
	}
	uint64_t startAddr = 0; //遍历起始位置
	uint64_t endAddr = 0; //遍历结束位置

	for (auto sec : vLibxxx_so) {

		if (startAddr == 0 && is_r0xp(&sec)) {
			startAddr = sec.baseaddress; //设置起始位置
			endAddr = startAddr + 0x7FFFFFFF; //设置结束位置，通常2G够用了
			break;
		}

		//uint64_t tmpEndAddr = sec.baseaddress + sec.size;
		//if (tmpEndAddr > endAddr) {
		//	endAddr = tmpEndAddr + startAddr; //遍历一个so模块的大小区域，请务必再三确认此值范围是否在你所要的目标范围内，否则搜索不到难查出问题
		//}

	}
	if (!startAddr || !endAddr) {
		printf("startAddr失败");
		return;
	}

	std::atomic<uint64_t> curWorkAddr{ startAddr }; //全局当前遍历位置

	struct OffsetResultInfo {
		uint64_t offset[5] = { 0 };
	};
	SafeVector<OffsetResultInfo> vResultInfo; //遍历结果

	
	MultiThreadExecuteTask(std::thread::hardware_concurrency(),
		[pRwDriver, &vSearchResult, &curWorkAddr, startAddr, endAddr, &vResultInfo](size_t thread_id, std::atomic<bool> *pForceStopSignal)->void
	{
		std::vector<ADDR_RESULT_INFO> vLastAddrResult;
		vSearchResult.copy_vals(vLastAddrResult);

		std::vector<OffsetResultInfo> vThreadOutput; //存放当前线程的搜索结果

		uint64_t curThreadAddr = 0;
		while (!pForceStopSignal || !*pForceStopSignal) {
			curThreadAddr = curWorkAddr.fetch_add(4);
			if (curThreadAddr >= endAddr) {
				break;
			}
			uint64_t addr1 = 0;
			if (!pRwDriver->ReadProcessMemory(1, curThreadAddr, &addr1, 8, NULL)) {
				continue;
			}

			for (uint64_t x2 = 0; x2 < 0x200; x2 += 4) {
				uint64_t addr2 = 0;
				if (!pRwDriver->ReadProcessMemory(1, addr1 + x2, &addr2, 8, NULL)) {
					continue;
				}
				if (addr2 < 0x7FFFFFFF) {
					continue;
				}

				for (uint64_t x3 = 0; x3 < 0x200; x3 += 4) {
					uint64_t addr3 = 0;
					if (!pRwDriver->ReadProcessMemory(1, addr2 + x3, &addr3, 8, NULL)) {
						continue;
					}
					if (addr3 < 0x7FFFFFFF) {
						continue;
					}

					for (uint64_t x4 = 0; x4 < 0x200; x4 += 4) {
						uint64_t addr4 = 0;
						if (!pRwDriver->ReadProcessMemory(1, addr3 + x4, &addr4, 8, NULL)) {
							continue;
						}
						if (addr4 < 0x7FFFFFFF) {
							continue;
						}

						//单个值为目标
	/*					uint64_t target = 0x7AF6E21600;
						if (addr4 > target) {
							continue;
						}
						uint64_t dis = target - addr4;
						if (dis > 0xFFF) {
							continue;
						}*/

						//多个值为目标
						uint64_t dis = 0;
						bool found = false;
						for (auto item : vLastAddrResult) {
							uint64_t target = item.addr;
							if (addr4 > target) {
								continue;
							}
							dis = target - addr4;
							if (dis > 0xFFF) {
								continue;
							}
							found = true;
							break;
						}
						if (!found) {
							continue;
						}

						OffsetResultInfo newOffset = { 0 };
						newOffset.offset[0] = curThreadAddr - startAddr;
						newOffset.offset[1] = x2;
						newOffset.offset[2] = x3;
						newOffset.offset[3] = x4;
						newOffset.offset[4] = dis;
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
		std::vector<OffsetResultInfo> vTmp;
		vResultInfo.copy_vals(vTmp);
		for (OffsetResultInfo & item : vTmp) {
			/* write some data to the file */
			fprintf(stream, "[0]->%x [1]->%x [2]->%x [3]->%x [4]->%x\n",
				item.offset[0],
				item.offset[1],
				item.offset[2],
				item.offset[3],
				item.offset[4]);
		}
		/* close the file */
		fclose(stream);
	}

}


int main(int argc, char *argv[])
{
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
	if (argc > 1)
	{
		//如果用户自定义输入驱动名
		devFileName = argv[1];
	}
	printf("Connecting rwDriver:%s\n", devFileName.c_str());


	//连接驱动
	int err = 0;
	if (!rwDriver.ConnectDriver(devFileName.c_str(), err))
	{
		printf("Connect rwDriver failed. error:%d\n", err);
		fflush(stdout);
		return 0;
	}



	//获取目标进程PID
	const char *name = "com.miui.calculator";
	pid_t pid = findPID(name, &rwDriver);
	if (pid == 0)
	{
		printf("找不到进程\n");
		fflush(stdout);
		return 0;
	}
	printf("目标进程PID:%d\n", pid);
	//打开进程
	uint64_t hProcess = rwDriver.OpenProcess(pid);
	printf("调用驱动 OpenProcess 返回值:%" PRIu64 "\n", hProcess);
	if (!hProcess)
	{
		printf("调用驱动 OpenProcess 失败\n");
		fflush(stdout);
		return 0;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//1.演示多线程正向搜索
	SafeVector<ADDR_RESULT_INFO> vSearchResult; //搜索结果
	normal_val_search(&rwDriver, hProcess, vSearchResult);
	//2.演示多线程反向搜索
	reverse_search(&rwDriver, hProcess, vSearchResult);
	//3.演示多线程遍历搜索
	loop_search(&rwDriver, hProcess, vSearchResult);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//关闭进程
	rwDriver.CloseHandle(hProcess);
	printf("调用驱动 CloseHandle:%" PRIu64 "\n", hProcess);
	fflush(stdout);
	return 0;
}