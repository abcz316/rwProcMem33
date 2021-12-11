#include <cstdio>
#include <string.h> 
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <memory>
#include <dirent.h>
#include <cinttypes>
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


//获取进程的内存块区域
BOOL GetMemRegion(CMemoryReaderWriter *pDriver, uint64_t hProcess, int type ,BOOL showPhy, std::vector<MEM_SECTION_INFO> & vOutput)
{
	//驱动_获取进程内存块地址列表
	std::vector<DRIVER_REGION_INFO> vMapsList;
	BOOL bOutListCompleted;
	pDriver->VirtualQueryExFull(hProcess, showPhy, vMapsList, bOutListCompleted);
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
			if (strstr(rinfo.name, "/dev/ashmem/") && !strstr(rinfo.name,"dalvik")) { vaild = 1; }
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


	//获取进程ANONMYOUS内存区域
	std::vector<MEM_SECTION_INFO> vScanMemMaps;
	GetMemRegion(&rwDriver, hProcess, 
		A_ANONMYOUS, 
		TRUE/*FALSE为全部内存，TRUE为只在物理内存中的内存*/, 
		vScanMemMaps);
	if (!vScanMemMaps.size())
	{
		printf("无内存可搜索\n");
		//关闭进程
		rwDriver.CloseHandle(hProcess);
		printf("调用驱动 CloseHandle:%" PRIu64 "\n", hProcess);
		fflush(stdout);
		return 0;
	}

	//首次搜索0.33333334327

	std::shared_ptr<CMemReaderWriterProxy> spReadWriteProxy = std::make_shared<CMemReaderWriterProxy>(&rwDriver);
	SafeVector<MEM_SECTION_INFO> vScanMemMapsList(vScanMemMaps); //搜索结果
	SafeVector<ADDR_RESULT_INFO> vSearchResult; //搜索结果
	SafeVector<MEM_SECTION_INFO> vSearcError;
	SafeVector<ADDR_RESULT_INFO> vSearcError2;

	//非阻塞模式
	//std::thread td1(SearchMemoryThread<float>,.....);
	//td1.detach();


	//阻塞模式
	SearchMemoryThread<float>(
		spReadWriteProxy,
		hProcess,
		vScanMemMapsList, //待搜索的内存区域
		0.33333334327f, //搜索数值
		0.0f,
		0.01, //误差范围
		SCAN_TYPE::ACCURATE_VAL, //搜索类型: 精确搜索
		std::thread::hardware_concurrency() - 1, //搜索线程数
		4, //快速扫描的对齐位数，CE默认为4
		vSearchResult, //搜索后的结果
		vSearcError);


	printf("共搜索出%zu个地址\n", vSearchResult.size());

	//将每个地址往后偏移20
	SafeVector<ADDR_RESULT_INFO> vWaitSearchAddr; //待搜索的内存地址列表

	ADDR_RESULT_INFO addr;
	while(vSearchResult.get_val(addr))
	{
		addr.addr += 20;
		vWaitSearchAddr.push_back(addr);
	}

	//再次搜索1.19175350666
	vSearchResult.clear();
	vSearcError.clear();
	vSearcError2.clear();

	SearchNextMemoryThread<float>(
		spReadWriteProxy,
		hProcess,
		vWaitSearchAddr, //待搜索的内存地址列表
		1.19175350666f, //搜索数值
		0, 
		0.01, //误差范围
		SCAN_TYPE::ACCURATE_VAL, //搜索类型: 精确搜索
		std::thread::hardware_concurrency() - 1, //搜索线程数
		vSearchResult,  //搜索后的结果
		vSearcError2);

	//将每个地址往后偏移76
	vWaitSearchAddr.clear();
	while(vSearchResult.get_val(addr))
	{
		addr.addr += 76;
		vWaitSearchAddr.push_back(addr);
	}

	
	//再次搜索-50.00500106812
	vSearchResult.clear();
	vSearcError.clear();
	vSearcError2.clear();
	SearchNextMemoryThread<float>(
		spReadWriteProxy,
		hProcess,
		vWaitSearchAddr,//待搜索的内存地址列表
		-50.00500106812f, //搜索数值
		0, 
		0.01, //误差范围
		SCAN_TYPE::ACCURATE_VAL, //搜索类型: 精确搜索
		std::thread::hardware_concurrency() - 1, //搜索线程数
		vSearchResult, //搜索后的结果
		vSearcError2);

	//将每个地址往后偏移952
	vWaitSearchAddr.clear();
	while (vSearchResult.get_val(addr))
	{
		addr.addr += 952;
		vWaitSearchAddr.push_back(addr);
	}

	//再次搜索-2147483648
	vSearchResult.clear();
	vSearcError.clear();
	vSearcError2.clear();
	SearchNextMemoryThread<int>(
		spReadWriteProxy,
		hProcess,
		vWaitSearchAddr,//待搜索的内存地址列表
		-2147483648, //搜索数值
		0,
		0,
		SCAN_TYPE::ACCURATE_VAL, //搜索类型: 精确搜索
		std::thread::hardware_concurrency() - 1, //搜索线程数
		vSearchResult, //搜索后的结果
		vSearcError2);

	size_t count = 0;
	while(vSearchResult.get_val(addr))
	{
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
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//关闭进程
	rwDriver.CloseHandle(hProcess);
	printf("调用驱动 CloseHandle:%" PRIu64 "\n", hProcess);
	fflush(stdout);
	return 0;
}