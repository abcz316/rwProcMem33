#ifndef MEM_SEARCH_HELPER_H_
#define MEM_SEARCH_HELPER_H_
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <sstream>
#include <sys/sysinfo.h>


#include "../testKo/MemoryReaderWriter37.h"

struct MEM_SECTION_INFO
{
	uint64_t npSectionAddr = 0; //进程内存地址
	uint64_t nSectionSize = 0; //区域大小
};
struct COPY_MEM_INFO
{
	uint64_t npSectionAddr = 0; //进程内存地址
	uint64_t nSectionSize = 0; //区域大小
	std::shared_ptr<unsigned char> spSaveMemBuf = nullptr; //保存到本地缓冲区的内存字节数据
};
struct ADDR_RESULT_INFO
{
	uint64_t addr = 0; //进程内存地址
	uint64_t size = 0; //大小
	std::shared_ptr<unsigned char> spSaveData = nullptr; //保存到本地缓冲区的内存字节数据
};



/*
内存搜索数值，成功返回true，失败返回false
hProcess：被搜索的进程句柄
vScanMemMaps: 被搜索的进程内存区域
value1: 要搜索的数值1
value2: 要搜索的数值2
errorRange: 误差范围（当搜索的数值为float或者double时此值生效）
scanType：搜索类型：0精确数值（value1生效）、1值大于（value1生效）、2值小于（value1生效）、3值在两值之间（value1、value2生效）
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
template<typename T> static void SearchMemoryThread(
	CMemoryReaderWriter *pDriver,
	uint64_t hProcess,
	std::vector<MEM_SECTION_INFO>vScanMemMaps,
	T value1, T value2, float errorRange, int scanType, int nThreadCount,
	size_t nFastScanAlignment,
	std::vector<ADDR_RESULT_INFO> * vOutputAddr,
	std::atomic<int> *nSearchProgress,
	std::atomic<uint64_t> *nFoundAddrCount);

/*
再次搜索内存数值，成功返回true，失败返回false
hProcess：被搜索的进程句柄
vScanMemAddrList: 需要再次搜索的内存地址列表
value1: 要搜索数值1
value2: 要搜索数值2
errorRange: 误差范围（当搜索的数值为float或者double时此值生效）
scanType：搜索类型：0精确数值（value1生效）、1值大于（value1生效）、2值小于（value1生效）、3值在两值之间（value1、value2生效）、5增加的数值（value1、value2均无效），6数值增加了（value1生效），7减少的数值（value1、value2均无效），8数值减少了（value1生效），9变动的数值（value1、value2均无效），10未变动的数值（value1、value2均无效）
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
vOutputAddr：存放搜索完成的结果地址
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
template<typename T> static void SearchNextMemoryThread(
	CMemoryReaderWriter *pDriver,
	uint64_t hProcess,
	std::vector<ADDR_RESULT_INFO>vScanMemAddrList,
	T value1, T value2, float errorRange, int scanType, int nThreadCount,
	std::vector<ADDR_RESULT_INFO> * vOutputAddr,
	std::atomic<int> *nSearchProgress,
	std::atomic<uint64_t> *nFoundAddrCount);

/*
搜索已拷贝的进程内存数值，成功返回true，失败返回false
vCopyProcessMemData: 需要再次搜索的已拷贝的进程内存列表
value1: 要搜索的数值1
value2: 要搜索的数值2
errorRange: 误差范围（当搜索的数值为float或者double时此值生效）
scanType：搜索类型：0精确数值（value1生效）、1值大于（value1生效）、2值小于（value1生效）、3值在两值之间（value1、value2生效）、5增加的数值（value1、value2均无效），6数值增加了（value1生效），7减少的数值（value1、value2均无效），8数值减少了（value1生效），9变动的数值（value1、value2均无效），10未变动的数值（value1、value2均无效）
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
vOutputAddr：存放搜索完成的结果地址
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
template<typename T> static void SearchCopyProcessMemThread(
	CMemoryReaderWriter *pDriver,
	uint64_t hProcess,
	std::vector<COPY_MEM_INFO> *vCopyProcessMemData,
	T value1, T value2, float errorRange, int scanType, int nThreadCount,
	size_t nFastScanAlignment,
	std::vector<ADDR_RESULT_INFO> * vOutputAddr,
	std::atomic<int> *nSearchProgress,
	std::atomic<uint64_t> *nFoundAddrCount);


/*
内存搜索（搜索文本特征码），成功返回true，失败返回false
hProcess：被搜索的进程句柄
vScanMemMaps: 被搜索的进程内存区域
strFeaturesByte: 十六进制的字节特征码，搜索规则与OD相同，如“68 00 00 00 40 ?3 7? ?? ?? ?? ?? ?? ?? 50 E8”
nThreadCount: 用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
static void SearchMemoryBytesThread(CMemoryReaderWriter *pDriver, uint64_t hProcess, std::vector<MEM_SECTION_INFO>vScanMemMaps, std::string strFeaturesByte, int nThreadCount, size_t nFastScanAlignment, std::vector<ADDR_RESULT_INFO> * vOutputAddr, std::atomic<int> *nSearchProgress, std::atomic<uint64_t> *nFoundAddrCount);


/*
内存搜索（搜索内存特征码），成功返回true，失败返回false
hProcess：被搜索的进程句柄
vScanMemMaps: 被搜索的进程内存区域
vFeaturesByte：字节特征码，如“char vBytes[] = {'\x68', '\x00', '\x00', '\x00', '\x40', '\x?3', '\x??', '\x7?', '\x??', '\x??', '\x??', '\x??', '\x??', '\x50', '\xE8};”
nFeaturesByteLen：字节特征码长度
vFuzzyBytes：模糊字节，不变动的位置用1表示，变动的位置用0表示，如“char vFuzzy[] = {'\x11', '\x11', '\x11', '\x11', '\x11', '\x01', '\x00', '\x10', '\x00', '\x00', '\x00', '\x00', '\x00', '\x11', '\x11};”
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
static void SearchMemoryBytesThread2(CMemoryReaderWriter *pDriver, uint64_t hProcess, std::vector<MEM_SECTION_INFO>vScanMemMaps, char vFeaturesByte[], size_t nFeaturesByteLen, char vFuzzyBytes[], int nThreadCount, size_t nFastScanAlignment, std::vector<ADDR_RESULT_INFO> * vOutputAddr, std::atomic<int> *nSearchProgress, std::atomic<uint64_t> *nFoundAddrCount);


/*
再次搜索内存（搜索文本特征码），成功返回true，失败返回false
hProcess：被搜索的进程句柄
vScanMemAddrList: 需要再次搜索的内存地址列表
strFeaturesByte: 十六进制的字节特征码，搜索规则与OD相同，如“68 00 00 00 40 ?3 7? ?? ?? ?? ?? ?? ?? 50 E8”
nThreadCount: 用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
static void SearchNextMemoryBytesThread(CMemoryReaderWriter *pDriver, uint64_t hProcess, std::vector<ADDR_RESULT_INFO>vScanMemAddrList, std::string strFeaturesByte, int nThreadCount, size_t nFastScanAlignment, std::vector<ADDR_RESULT_INFO> * vOutputAddr, std::atomic<int> *nSearchProgress, std::atomic<uint64_t> *nFoundAddrCount);



/*
再次搜索内存（搜索内存特征码），成功返回true，失败返回false
hProcess：被搜索的进程句柄
vScanMemAddrList: 需要再次搜索的内存地址列表
vFeaturesByte：字节特征码，如“char vBytes[] = {0x68, 0x00, 0x00, 0x00, 0x40, 0x?3, 0x??, 0x7?, 0x??, 0x??, 0x??, 0x??, 0x??, 0x50, 0xE8};”
nFeaturesByteLen：字节特征码长度
vFuzzyBytes：模糊字节，不变动的位置用1表示，变动的位置用0表示，如“char vFuzzy[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11};”
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
static void SearchNextMemoryBytesThread2(CMemoryReaderWriter *pDriver, uint64_t hProcess, std::vector<ADDR_RESULT_INFO>vScanMemAddrList, char vFeaturesByte[], size_t nFeaturesByteLen, char vFuzzyBytes[], int nThreadCount, size_t nFastScanAlignment, std::vector<ADDR_RESULT_INFO> * vOutputAddr, std::atomic<int> *nSearchProgress, std::atomic<uint64_t> *nFoundAddrCount);

/*
拷贝进程内存数据，成功返回true，失败返回false
hProcess：被搜索的进程句柄
vScanMemMaps: 被拷贝的进程内存区域
vOutputMemCopy：拷贝进程内存数据的存放区域
scanValueType：记录本次拷贝进程内存数据的值类型，1:1字节，2:2字节，4:4字节，8:8字节，9:单浮点，10:双浮点
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
static void CopyProcessMemDataThread(CMemoryReaderWriter *pDriver, uint64_t hProcess, std::vector<MEM_SECTION_INFO>vScanMemMaps, std::vector<COPY_MEM_INFO> * vOutputMemCopy, int scanValueType, std::atomic<int> *nSearchProgress, std::atomic<uint64_t> *nFoundAddrCount);





/*
寻找精确数值
使用方法:
dwAddr：要搜索的缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索的数值
errorRange: 误差范围（当搜索的数值为float或者double时此值生效）
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindValue(size_t dwAddr, size_t dwLen, T value, float errorRange, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount);

/*
寻找大于的数值
使用方法:
dwAddr：要搜索的缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索大于的数值
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindMax(size_t dwAddr, size_t dwLen, T value, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount);

/*
寻找小于的数值
使用方法:
dwAddr：要搜索的缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索小于的char数值
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindMin(size_t dwAddr, size_t dwLen, T value, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount);

/*
寻找两者之间的数值
使用方法:
dwAddr：要搜索的缓冲区地址
dwLen：要搜索的缓冲区大小
value1: 要搜索的数值1
value2: 要搜索的数值2
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindBetween(size_t dwAddr, size_t dwLen, T value1, T value2, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount);

/*
寻找增加的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindUnknowAdd(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount);

/*
寻找增加了已知值的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索增加的已知值
errorRange: 误差范围（当搜索的数值为float或者double时此值生效）
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindAdd(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, T value, float errorRange, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount);

/*
寻找减少的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindUnknowSum(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount);

/*
寻找减少了已知值的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索减少的已知值
errorRange: 误差范围（当搜索的数值为float或者double时此值生效）
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindSum(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, T value, float errorRange, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount);

/*
寻找变动的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindChanged(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount);

/*
寻找未变动的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindNoChange(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount);



/*
寻找字节集
dwAddr: 要搜索的缓冲区地址
dwLen: 要搜索的缓冲区长度
bMask: 要搜索的字节集
szMask: 要搜索的字节集模糊码（xx=已确定字节，??=可变化字节，支持x?、?x）
nMaskLen: 要搜索的字节集长度
nFastScanAlignment: 为快速扫描的对齐位数，CE默认为1
vOutputAddr: 搜索出来的结果地址存放数组
nFoundAddrCount: 实时找到的地址数量
使用方法：
std::vector<size_t> vAddr;
std::atomic<uint64_t> nFoundAddrCount;
nFoundAddrCount = 0;
FindFeaturesBytes((size_t)lpBuffer, dwBufferSize, (PBYTE);"\x8B\xE8\x00\x00\x00\x00\x33\xC0\xC7\x06\x00\x00\x00\x00\x89\x86\x40", "xxxx??xx?xx?xxxxx",17,1,vAddr , nFoundAddrCount);;
*/
static inline void FindFeaturesBytes(size_t dwAddr, size_t dwLen, unsigned char *bMask, const char* szMask, size_t nMaskLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount);

/*
寻找字节集
dwWaitSearchAddress: 要搜索的缓冲区地址
dwLen: 要搜索的缓冲区长度
bForSearch: 要搜索的字节集
ifLen: 要搜索的字节集长度
nFastScanAlignment: 为快速扫描的对齐位数，CE默认为1
vOutputAddr: 搜索出来的结果地址存放数组
nFoundAddrCount: 实时找到的地址数量
使用方法：
std::vector<size_t> vAddr;
std::atomic<uint64_t> nFoundAddrCount;
nFoundAddrCount = 0;
FindFeaturesBytes((size_t)lpBuffer, dwBufferSize, (PBYTE)"\x8B\xE8\x00\x00\x00\x00\x33\xC0\xC7\x06\x00\x00\x00\x00\x89\x86\x40", 17,1,vAddr , nFoundAddrCount);
*/
static inline void FindBytes(size_t dwWaitSearchAddress, size_t dwLen, unsigned char *bForSearch, size_t ifLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount);

static inline std::string& replace_all_distinct(std::string& str, const std::string& old_value, const std::string& new_value);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/*
内存搜索数值，成功返回true，失败返回false
hProcess：被搜索的进程句柄
vScanMemMaps: 被搜索的进程内存区域
value1: 要搜索的数值1
value2: 要搜索的数值2
errorRange: 误差范围（当搜索的数值为float或者double时此值生效）
scanType：搜索类型：0精确数值（value1生效）、1值大于（value1生效）、2值小于（value1生效）、3值在两值之间（value1、value2生效）
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
template<typename T> static void SearchMemoryThread(
	CMemoryReaderWriter *pDriver, 
	uint64_t hProcess, 
	std::vector<MEM_SECTION_INFO>vScanMemMaps, 
	T value1, T value2, float errorRange, int scanType, int nThreadCount,
	size_t nFastScanAlignment, 
	std::vector<ADDR_RESULT_INFO> * vOutputAddr, 
	std::atomic<int> *nSearchProgress, 
	std::atomic<uint64_t> *nFoundAddrCount)
{
	//获取当前系统内存大小
	struct sysinfo si;
	sysinfo(&si);
	size_t nMaxMemSize = si.totalram;


	std::mutex mtxResultAccessLock;			//搜索结果汇总数组访问锁
	std::vector<ADDR_RESULT_INFO> vResultAddr;	//全部线程的搜索结果汇总数组

	//分配线程进行内存搜索
	size_t nScanMemMapsMaxCount = vScanMemMaps.size(); //记录被搜索的进程内存区域总数
	size_t nThreadJob = nScanMemMapsMaxCount / nThreadCount; //每条线程需要处理的内存区段任务数
	//cout << "每 " << nThreadJob << endl;
	if (nThreadJob == 0)
	{
		nThreadJob = 1;
		nThreadCount = nScanMemMapsMaxCount;
	}

	std::atomic<int> nWorkingThreadCount; //记录实时正在工作的搜索线程数
	std::atomic<uint64_t> nFinishedMemSectionJobCount; //记录实时已处理完成的内存区段任务总数
	nWorkingThreadCount = nThreadCount;
	nFinishedMemSectionJobCount = 0;

	//开始分配线程搜索内存
	for (int i = 0; i < nThreadCount; i++)
	{
		//每条线程需要处理的内存区段任务数组
		std::unique_ptr<std::vector<MEM_SECTION_INFO>> upVecThreadJob = std::make_unique<std::vector<MEM_SECTION_INFO>>();


		for (int count = 0; count < nThreadJob; count++)
		{
			MEM_SECTION_INFO task = *vScanMemMaps.rbegin(); //从总内存区段任务队列末尾取出一个内存区段任务
			upVecThreadJob->push_back(task); //取完保存起来
			vScanMemMaps.pop_back(); //取完末尾后删除
		}

		if (i + 1 == nThreadCount)
		{
			//如果是最后一条线程了，则需要把剩下的内存区段任务全部处理
			for (MEM_SECTION_INFO task : vScanMemMaps)
			{
				upVecThreadJob->push_back(task);
			}
		}


		//内存搜索线程
		std::thread td(
			[](
				CMemoryReaderWriter *pDriver,
				size_t nMaxMallocMem, //最大申请内存字节
				std::unique_ptr<std::vector<MEM_SECTION_INFO>> upVecThreadJob, //每条线程需要处理的内存区段任务数组
				uint64_t hProcess, //被搜索进程句柄
				std::atomic<int> *nWorkingThreadCount, //记录实时正在工作的搜索线程数
				std::atomic<uint64_t> *nFinishedMemSectionJobCount, //记录实时已处理完成的内存区段任务总数
				std::atomic<uint64_t> *nFoundAddrCount, //记录实时已找到的地址数目
				T value1, //要搜索的char数值1
				T value2, //要搜索的char数值2
				float errorRange,
				int scanType, //搜索类型：0精确数值（value1生效）、1值大于（value1生效）、2值小于（value1生效）、3值在两值之间（value1、value2生效）
				std::mutex *mtxResultAccessLock,	//搜索结果汇总数组访问锁
				std::vector<ADDR_RESULT_INFO> *pvResultAddr,	//全部线程的搜索结果汇总数组
				size_t nFastScanAlignment	//快速扫描的对齐位数
				)->void
		{
			//cout << "我 " << upVecThreadJob->size() << endl;
			std::vector<ADDR_RESULT_INFO> vThreadOutput; //存放当前线程的搜索结果

			for (MEM_SECTION_INFO memSecInfo : *upVecThreadJob) //线程的每个任务
			{
				//当前内存区域块大小超过了最大内存申请的限制，所以跳过
				if (memSecInfo.nSectionSize > nMaxMallocMem) { continue; }
				
				unsigned char *pnew = new (std::nothrow) unsigned char[memSecInfo.nSectionSize];
				if (!pnew) { /*cout << "malloc "<< memSecInfo.nSectionSize  << " failed."<< endl;*/ continue; }
				memset(pnew, 0, memSecInfo.nSectionSize);


				std::shared_ptr<unsigned char> spMemBuf(pnew, std::default_delete<unsigned char[]>());
				size_t dwRead = 0;
				if (!pDriver->ReadProcessMemory(hProcess, memSecInfo.npSectionAddr, spMemBuf.get(), memSecInfo.nSectionSize, &dwRead, FALSE))
				{
					continue;
				}
				//寻找数值
				std::vector<size_t >vFindAddr;

				switch (scanType)
				{
				case 0:
					//精确数值
					FindValue<T>((size_t)spMemBuf.get(), dwRead, value1, errorRange, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
					break;
				case 1:
					//值大于
					FindMax<T>((size_t)spMemBuf.get(), dwRead, value1, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
					break;
				case 2:
					//值小于
					FindMin<T>((size_t)spMemBuf.get(), dwRead, value1, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
					break;
				case 3:
					//值在两者之间于
					FindBetween<T>((size_t)spMemBuf.get(), dwRead, value1 < value2 ? value1 : value2, value1 < value2 ? value2 : value1, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
					break;
				default:
					break;
				}

			
				for (size_t addr : vFindAddr)
				{
					ADDR_RESULT_INFO aInfo;
					aInfo.addr = (uint64_t)((uint64_t)memSecInfo.npSectionAddr + (uint64_t)addr - (uint64_t)spMemBuf.get());
					aInfo.size = 1;

					std::shared_ptr<unsigned char> sp(new unsigned char[aInfo.size], std::default_delete<unsigned char[]>());
					memcpy(sp.get(), (void*)addr, aInfo.size);

					aInfo.spSaveData = sp;
					vThreadOutput.push_back(aInfo);
				}
				(*nFinishedMemSectionJobCount)++; //实时已处理完成的内存区段任务总数+1
			}
			//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
			mtxResultAccessLock->lock();
			for (ADDR_RESULT_INFO newAddr : vThreadOutput)
			{
				pvResultAddr->push_back(newAddr);
			}
			mtxResultAccessLock->unlock();

			(*nWorkingThreadCount)--; //实时正在工作的搜索线程数-1
			return;

		},
			pDriver,
			nMaxMemSize,
			std::move(upVecThreadJob),
			hProcess,
			&nWorkingThreadCount,
			&nFinishedMemSectionJobCount,
			nFoundAddrCount,
			value1,
			value2,
			errorRange,
			scanType,
			&mtxResultAccessLock,
			&vResultAddr,
			nFastScanAlignment
			);
		td.detach();

	}
	//等待所有搜索线程结束汇总
	while (nWorkingThreadCount != 0)
	{
		//计算搜索进度为
		uint64_t progress = ((double)nFinishedMemSectionJobCount / (double)nScanMemMapsMaxCount) * 100;
		if (progress >= 100)
		{
			(*nSearchProgress) = 99;
		}
		else
		{
			(*nSearchProgress) = progress;
		}

		sleep(0);
	}

	vOutputAddr->clear();
	vOutputAddr->assign(vResultAddr.begin(), vResultAddr.end());
	sort(vOutputAddr->begin(), vOutputAddr->end(), [](ADDR_RESULT_INFO a, ADDR_RESULT_INFO b) -> bool { return a.addr < b.addr; });

	//设置搜索进度为100
	(*nSearchProgress) = 100;
	return;
}

/*
再次搜索内存数值，成功返回true，失败返回false
hProcess：被搜索的进程句柄
vScanMemAddrList: 需要再次搜索的内存地址列表
value1: 要搜索数值1
value2: 要搜索数值2
errorRange: 误差范围（当搜索的数值为float或者double时此值生效）
scanType：搜索类型：0精确数值（value1生效）、1值大于（value1生效）、2值小于（value1生效）、3值在两值之间（value1、value2生效）、5增加的数值（value1、value2均无效），6数值增加了（value1生效），7减少的数值（value1、value2均无效），8数值减少了（value1生效），9变动的数值（value1、value2均无效），10未变动的数值（value1、value2均无效）
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
vOutputAddr：存放搜索完成的结果地址
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
template<typename T> static void SearchNextMemoryThread(
	CMemoryReaderWriter *pDriver,
	uint64_t hProcess, 
	std::vector<ADDR_RESULT_INFO>vScanMemAddrList,
	T value1, T value2, float errorRange, int scanType, int nThreadCount,
	std::vector<ADDR_RESULT_INFO> * vOutputAddr, 
	std::atomic<int> *nSearchProgress,
	std::atomic<uint64_t> *nFoundAddrCount)
{
	std::mutex mtxResultAccessLock;			//搜索结果汇总数组访问锁
	std::vector<ADDR_RESULT_INFO> vResultAddr;	//全部线程的搜索结果汇总数组

	//分配线程进行内存搜索
	size_t nScanMemAddrMaxCount = vScanMemAddrList.size(); //记录需要再次搜索的内存地址总数
	size_t nThreadJob = nScanMemAddrMaxCount / nThreadCount; //每条线程需要处理的内存地址任务数
	//cout << "每 " << nThreadJob << endl;
	if (nThreadJob == 0)
	{
		nThreadJob = 1;
		nThreadCount = nScanMemAddrMaxCount;
	}

	std::atomic<int> nWorkingThreadCount; //记录实时正在工作的搜索线程数
	std::atomic<uint64_t> nFinishedMemAddrJobCount{0}; //记录实时已处理完成的内存地址任务总数
	nWorkingThreadCount = nThreadCount;


	//开始分配线程搜索内存
	for (int i = 0; i < nThreadCount; i++)
	{
		//每条线程需要处理的内存地址任务数组
		std::unique_ptr<std::vector<ADDR_RESULT_INFO>> upVecThreadJob = std::make_unique<std::vector<ADDR_RESULT_INFO>>();


		for (int count = 0; count < nThreadJob; count++)
		{
			ADDR_RESULT_INFO task = *vScanMemAddrList.rbegin(); //从总内存区段任务队列末尾取出一个内存地址任务
			upVecThreadJob->push_back(task); //取完保存起来
			vScanMemAddrList.pop_back(); //取完末尾后删除
		}

		if (i + 1 == nThreadCount)
		{
			//如果是最后一条线程了，则需要把剩下的内存地址任务全部处理
			for (ADDR_RESULT_INFO task : vScanMemAddrList)
			{
				upVecThreadJob->push_back(task);
			}
		}

		//内存搜索线程
		std::thread td(
			[](
				CMemoryReaderWriter *pDriver,
				std::unique_ptr<std::vector<ADDR_RESULT_INFO>> upVecThreadJob, //每条线程需要处理的内存地址任务数组
				uint64_t hProcess, //被搜索进程句柄
				std::atomic<int> *nWorkingThreadCount, //记录实时正在工作的搜索线程数
				std::atomic<uint64_t> *nFinishedMemAddrJobCount, //记录实时已处理完成的内存地址任务总数
				std::atomic<uint64_t> *nFoundAddrCount, //记录实时已找到的地址数目
				T value1, //要搜索的数值1
				T value2, //要搜索的数值2
				float errorRange, //误差范围
				int scanType, //搜索类型：0精确数值（value1生效）、1值大于（value1生效）、2值小于（value1生效）、3值在两值之间（value1、value2生效）、5增加的数值（value1、value2均无效），6数值增加了（value1生效），7减少的数值（value1、value2均无效），8数值减少了（value1生效），9变动的数值（value1、value2均无效），10未变动的数值（value1、value2均无效）
				std::mutex *mtxResultAccessLock,	//搜索结果汇总数组访问锁
				std::vector<ADDR_RESULT_INFO> *pvResultAddr	//全部线程的搜索结果汇总数组
				)->void
		{
			//cout << "我 " << upVecThreadJob->size() << endl;
			std::vector<ADDR_RESULT_INFO> vThreadOutput; //存放当前线程的搜索结果

			for (ADDR_RESULT_INFO memAddrJob : *upVecThreadJob) //线程的每个任务
			{
				(*nFinishedMemAddrJobCount)++; //实时已处理完成的内存区段任务总数+1

				T temp = 0;
				size_t dwRead = 0;
				if (!pDriver->ReadProcessMemory(hProcess, memAddrJob.addr, &temp, sizeof(temp), &dwRead, FALSE))
				{
					continue;
				}
				
				if (dwRead != sizeof(T)){ continue; }

				//寻找数值
				switch (scanType)
				{
				case 0:
					if (typeid(T) == typeid(static_cast<float>(0)) || typeid(T) == typeid(static_cast<double>(0)))
					{
						//当是float、double数值的情况
						if ((value1 - errorRange) > temp || temp > (value1 + errorRange)) { continue; }
					}
					else
					{
						//精确数值
						if (temp != value1) { continue; }
					}
					break;
				case 1:
					//值大于的结果保留
					if (temp < value1) { continue; }
					break;
				case 2:
					//值小于的结果保留
					if (temp > value1) { continue; }
					break;
				case 3:
					//值在两者之间的结果保留
					if (value1 < value2)
					{
						if (value1 > temp || temp > value2) { continue; }
					}
					else
					{
						if (value2 > temp || temp > value1) { continue; }
					}
					break;
				case 5:
					//增加的数值的结果保留
					if (temp <= *(T*)(memAddrJob.spSaveData.get())) { continue; }
					break;
				case 6:
					if (typeid(T) == typeid(static_cast<float>(0)) || typeid(T) == typeid(static_cast<double>(0)))
					{
						//float、double数值增加了的结果保留
						T* pOldData = (T*)&(*memAddrJob.spSaveData);
						T add = (temp)-(*pOldData);

						if ((value1 - errorRange) > add || add > (value1 + errorRange)) { continue; }
					}
					else
					{
						//数值增加了的结果保留
						if ( ((temp)-(*(T*)(memAddrJob.spSaveData.get()))) != value1) { continue; }

					}
					break;
				case 7:
					//减少的数值的结果保留
					if (temp >= *(T*)(memAddrJob.spSaveData.get())) { continue; }
					break;
				case 8:
					if (typeid(T) == typeid(static_cast<float>(0)) || typeid(T) == typeid(static_cast<double>(0)))
					{
						//float、double数值减少了的结果保留
						T* pOldData = (T*)&(*memAddrJob.spSaveData);
						T sum = (*pOldData) - (temp);

						if ((value1 - errorRange) > sum || sum > (value1 + errorRange)) { continue; }

					}
					else
					{
						//数值减少了的结果保留
						if (((*(T*)(memAddrJob.spSaveData.get())) - (temp)) != value1) { continue; }
					}
					break;
				case 9:
					if (typeid(T) == typeid(static_cast<float>(0)) || typeid(T) == typeid(static_cast<double>(0)))
					{
						//float、double变动的数值的结果保留
						T* pOldData = (T*)memAddrJob.spSaveData.get();
						if (temp > (*pOldData))
						{
							if ((temp - errorRange) <= (*pOldData)) { continue; }
						}
						if (temp < (*pOldData))
						{
							if ((temp + errorRange) >= (*pOldData)) { continue; }
						}

					}
					else
					{
						//变动的数值的结果保留
						if (temp == *(T*)(memAddrJob.spSaveData.get())) { continue; }
					}
					break;
				case 10:
					if (typeid(T) == typeid(static_cast<float>(0)) || typeid(T) == typeid(static_cast<double>(0)))
					{
						//float、double未变动的数值的结果保留
						T* pOldData = (T*)memAddrJob.spSaveData.get();

						if (temp > (*pOldData))
						{
							if ((temp - errorRange) > (*pOldData)) { continue; }
						}
						if (temp < (*pOldData))
						{
							if ((temp + errorRange) < (*pOldData)) { continue; }
						}

					}
					else
					{
						//未变动的数值的结果保留
						if (temp != *(T*)(memAddrJob.spSaveData.get())) { continue; }
					}

					break;
				default:
					break;
				}
			
			
				if (memAddrJob.size != 1)
				{
					memAddrJob.size = 1;
					std::shared_ptr<unsigned char> sp(new unsigned char[memAddrJob.size], std::default_delete<unsigned char[]>());
					memAddrJob.spSaveData = sp;
				}
				memcpy(&(*memAddrJob.spSaveData), &temp, 1);
				vThreadOutput.push_back(memAddrJob);

				(*nFoundAddrCount)++;

			}
			//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
			mtxResultAccessLock->lock();
			for (ADDR_RESULT_INFO newAddr : vThreadOutput)
			{
				pvResultAddr->push_back(newAddr);
			}
			mtxResultAccessLock->unlock();


			(*nWorkingThreadCount)--; //实时正在工作的搜索线程数-1
			return;

		},
			pDriver,
			std::move(upVecThreadJob),
			hProcess,
			&nWorkingThreadCount,
			&nFinishedMemAddrJobCount,
			nFoundAddrCount,
			value1,
			value2,
			errorRange,
			scanType,
			&mtxResultAccessLock,
			&vResultAddr
			);
		td.detach();

	}
	//等待所有搜索线程结束汇总
	while (nWorkingThreadCount != 0)
	{
		//计算搜索进度为
		uint64_t progress = ((double)nFinishedMemAddrJobCount / (double)nScanMemAddrMaxCount) * 100;
		if (progress >= 100)
		{
			(*nSearchProgress) = 99;
		}
		else
		{
			(*nSearchProgress) = progress;
		}

		sleep(0);
	}

	vOutputAddr->clear();
	vOutputAddr->assign(vResultAddr.begin(), vResultAddr.end());
	sort(vOutputAddr->begin(), vOutputAddr->end(), [](ADDR_RESULT_INFO a, ADDR_RESULT_INFO b) -> bool { return a.addr < b.addr; });
	//设置搜索进度为100
	(*nSearchProgress) = 100;

	return;

}

/*
搜索已拷贝的进程内存数值，成功返回true，失败返回false
vCopyProcessMemData: 需要再次搜索的已拷贝的进程内存列表
value1: 要搜索的数值1
value2: 要搜索的数值2
errorRange: 误差范围（当搜索的数值为float或者double时此值生效）
scanType：搜索类型：0精确数值（value1生效）、1值大于（value1生效）、2值小于（value1生效）、3值在两值之间（value1、value2生效）、5增加的数值（value1、value2均无效），6数值增加了（value1生效），7减少的数值（value1、value2均无效），8数值减少了（value1生效），9变动的数值（value1、value2均无效），10未变动的数值（value1、value2均无效）
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
vOutputAddr：存放搜索完成的结果地址
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
template<typename T> static void SearchCopyProcessMemThread(
	CMemoryReaderWriter *pDriver, 
	uint64_t hProcess, 
	std::vector<COPY_MEM_INFO> *vCopyProcessMemData, 
	T value1, T value2, float errorRange, int scanType, int nThreadCount,
	size_t nFastScanAlignment, 
	std::vector<ADDR_RESULT_INFO> * vOutputAddr, 
	std::atomic<int> *nSearchProgress, 
	std::atomic<uint64_t> *nFoundAddrCount)
{
	std::mutex mtxResultAccessLock;			//搜索结果汇总数组访问锁
	std::vector<ADDR_RESULT_INFO> vResultAddr;	//全部线程的搜索结果汇总数组

	//分配线程进行内存搜索
	size_t nCopyProcessMemMaxCount = vCopyProcessMemData->size(); //记录被搜索的进程内存区域总数
	size_t nThreadJob = nCopyProcessMemMaxCount / nThreadCount; //每条线程需要处理的内存区段任务数
	//cout << "每 " << nThreadJob << endl;
	if (nThreadJob == 0)
	{
		nThreadJob = 1;
		nThreadCount = nCopyProcessMemMaxCount;
	}

	std::atomic<int> nWorkingThreadCount; //记录实时正在工作的搜索线程数
	std::atomic<uint64_t> nFinishedMemSectionJobCount; //记录实时已处理完成的内存区段任务总数
	nWorkingThreadCount = nThreadCount;
	nFinishedMemSectionJobCount = 0;

	//开始分配线程搜索内存
	for (int i = 0; i < nThreadCount; i++)
	{
		//每条线程需要处理的内存区段任务数组
		std::unique_ptr<std::vector<COPY_MEM_INFO>> upVecThreadJob = std::make_unique<std::vector<COPY_MEM_INFO>>();


		for (int count = 0; count < nThreadJob; count++)
		{
			COPY_MEM_INFO task = *vCopyProcessMemData->rbegin(); //从总内存区段任务队列末尾取出一个内存区段任务
			upVecThreadJob->push_back(task); //取完保存起来
			vCopyProcessMemData->pop_back(); //取完末尾后删除
		}

		if (i + 1 == nThreadCount)
		{
			//如果是最后一条线程了，则需要把剩下的内存区段任务全部处理
			for (COPY_MEM_INFO task : *vCopyProcessMemData)
			{
				upVecThreadJob->push_back(task);
			}
		}

		//内存搜索线程
		std::thread td(
			[](
				CMemoryReaderWriter *pDriver,
				std::unique_ptr<std::vector<COPY_MEM_INFO>> upVecThreadJob, //每条线程需要处理的内存区段任务数组
				uint64_t hProcess, //被搜索进程句柄
				std::atomic<int> *nWorkingThreadCount, //记录实时正在工作的搜索线程数
				std::atomic<uint64_t> *nFinishedMemSectionJobCount, //记录实时已处理完成的内存区段任务总数
				std::atomic<uint64_t> *nFoundAddrCount, //记录实时已找到的地址数目
				T value1, //要搜索的int数值1
				T value2, //要搜索的int数值2
				float errorRange, //误差范围
				int scanType, //搜索类型：0精确数值（value1生效）、1值大于（value1生效）、2值小于（value1生效）、3值在两值之间（value1、value2生效）、5增加的数值（value1、value2均无效），6数值增加了（value1生效），7减少的数值（value1、value2均无效），8数值减少了（value1生效），9变动的数值（value1、value2均无效），10未变动的数值（value1、value2均无效）
				std::mutex *mtxResultAccessLock,	//搜索结果汇总数组访问锁
				std::vector<ADDR_RESULT_INFO> *pvResultAddr,	//全部线程的搜索结果汇总数组
				size_t nFastScanAlignment	//快速扫描的对齐位数
				)->void
		{
			//cout << "我 " << upVecThreadJob->size() << endl;
			std::vector<ADDR_RESULT_INFO> vThreadOutput; //存放当前线程的搜索结果

			for (COPY_MEM_INFO memSecInfo : *upVecThreadJob) //线程的每个任务
			{
				(*nFinishedMemSectionJobCount)++; //实时已处理完成的内存区段任务总数+1

				//寻找数值
				std::vector<size_t >vFindAddr;
				std::shared_ptr<unsigned char> spMemBuf = nullptr;
				size_t dwRead = 0;
				switch (scanType)
				{
				case 0:
					//精确数值
					FindValue<T>((size_t)(memSecInfo.spSaveMemBuf.get()),
						memSecInfo.nSectionSize, value1, errorRange, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
					break;
				case 1:
					//值大于
					FindMax<T>((size_t)(memSecInfo.spSaveMemBuf.get()),
						memSecInfo.nSectionSize, value1, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
					break;
				case 2:
					//值小于
					FindMin<T>((size_t)(memSecInfo.spSaveMemBuf.get()),
						memSecInfo.nSectionSize, value1, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
					break;
				case 3:
					//值在两者之间于
					FindBetween<T>((size_t)(memSecInfo.spSaveMemBuf.get()),
						memSecInfo.nSectionSize, value1 < value2 ? value1 : value2, value1 < value2 ? value2 : value1, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
					break;
				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:{
					unsigned char *pnew = new (std::nothrow) unsigned char[memSecInfo.nSectionSize];
					if (!pnew) { /*cout << "malloc "<< memSecInfo.nSectionSize  << " failed."<< endl;*/ continue; }
					memset(pnew, 0, memSecInfo.nSectionSize);

					std::shared_ptr<unsigned char> sp(pnew, std::default_delete<unsigned char[]>());

					if (!pDriver->ReadProcessMemory(hProcess, memSecInfo.npSectionAddr, sp.get(), memSecInfo.nSectionSize, &dwRead, FALSE))
					{
						continue;
					}
					spMemBuf = sp;
					break;}
				default:
					break;
				}

				switch (scanType)
				{
				case 5:
					//增加的数值
					FindUnknowAdd<T>((size_t)(memSecInfo.spSaveMemBuf.get()), (size_t)spMemBuf.get(), dwRead, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
					break;
				case 6:
					//数值增加了
					FindAdd<T>((size_t)memSecInfo.spSaveMemBuf.get(), (size_t)spMemBuf.get(), value1, errorRange, dwRead, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
					break;
				case 7:
					//减少的数值
					FindUnknowSum<T>((size_t)memSecInfo.spSaveMemBuf.get(), (size_t)spMemBuf.get(), dwRead, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
					break;
				case 8:
					//数值减少了
					FindSum<T>((size_t)memSecInfo.spSaveMemBuf.get(), (size_t)spMemBuf.get(), value1, errorRange, dwRead, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
					break;
				case 9:
					//变动的数值
					FindChanged<T>((size_t)memSecInfo.spSaveMemBuf.get(), (size_t)spMemBuf.get(), dwRead, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
					break;
				case 10:
					//未变动的数值
					FindNoChange<T>((size_t)memSecInfo.spSaveMemBuf.get(), (size_t)spMemBuf.get(), dwRead, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
					break;
				default:
					break;
				}
		
				for (size_t addr : vFindAddr)
				{
					ADDR_RESULT_INFO aInfo;
					aInfo.addr = (uint64_t)((uint64_t)memSecInfo.npSectionAddr + (uint64_t)addr - (uint64_t)memSecInfo.spSaveMemBuf.get());
					aInfo.size = 1;
					std::shared_ptr<unsigned char> sp(new unsigned char[aInfo.size], std::default_delete<unsigned char[]>());
					memcpy(sp.get(), (void*)addr, aInfo.size);
					aInfo.spSaveData = sp;
					vThreadOutput.push_back(aInfo);
				}
			}
			//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
			mtxResultAccessLock->lock();
			for (ADDR_RESULT_INFO newAddr : vThreadOutput)
			{
				pvResultAddr->push_back(newAddr);
			}
			mtxResultAccessLock->unlock();

			(*nWorkingThreadCount)--; //实时正在工作的搜索线程数-1
			return;

		},
			pDriver,
			std::move(upVecThreadJob),
			hProcess,
			&nWorkingThreadCount,
			&nFinishedMemSectionJobCount,
			nFoundAddrCount,
			value1,
			value2,
			errorRange,
			scanType,
			&mtxResultAccessLock,
			&vResultAddr,
			nFastScanAlignment
			);
		td.detach();

	}
	//等待所有搜索线程结束汇总
	while (nWorkingThreadCount != 0)
	{
		//计算搜索进度为
		uint64_t progress = ((double)nFinishedMemSectionJobCount / (double)nCopyProcessMemMaxCount) * 100;
		if (progress >= 100)
		{
			(*nSearchProgress) = 99;
		}
		else
		{
			(*nSearchProgress) = progress;
		}

		sleep(0);
	}

	vOutputAddr->clear();
	vOutputAddr->assign(vResultAddr.begin(), vResultAddr.end());
	sort(vOutputAddr->begin(), vOutputAddr->end(), [](ADDR_RESULT_INFO a, ADDR_RESULT_INFO b) -> bool { return a.addr < b.addr; });

	vCopyProcessMemData->clear();

	//设置搜索进度为100
	(*nSearchProgress) = 100;

	return;

}



/*
内存搜索（搜索文本特征码），成功返回true，失败返回false
hProcess：被搜索的进程句柄
vScanMemMaps: 被搜索的进程内存区域
strFeaturesByte: 十六进制的字节特征码，搜索规则与OD相同，如“68 00 00 00 40 ?3 7? ?? ?? ?? ?? ?? ?? 50 E8”
nThreadCount: 用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
static void SearchMemoryBytesThread(CMemoryReaderWriter *pDriver, uint64_t hProcess, std::vector<MEM_SECTION_INFO>vScanMemMaps, std::string strFeaturesByte, int nThreadCount, size_t nFastScanAlignment, std::vector<ADDR_RESULT_INFO> * vOutputAddr, std::atomic<int> *nSearchProgress, std::atomic<uint64_t> *nFoundAddrCount)
{


	//预处理特征码
	replace_all_distinct(strFeaturesByte, " ", "");//去除空格
	if (strFeaturesByte.length() % 2)
	{
		//设置搜索进度为100
		(*nSearchProgress) = 100;
		return;
	}

	//特征码字节数组
	std::unique_ptr<char[]> upFeaturesBytesBuf = std::make_unique<char[]>(strFeaturesByte.length() / 2); //如：68 00 00 00 40 ?3 7? ?? ?? ?? ?? ?? ?? 50 E8

	//特征码容错数组
	std::unique_ptr<char[]> upFuzzyBytesBuf = std::make_unique<char[]>(strFeaturesByte.length() / 2);

	size_t nFeaturesBytePos = 0; //记录写入特征码字节数组的位置

	//遍历每个文本字节
	for (std::string::size_type pos(0); pos < strFeaturesByte.length(); pos += 2)
	{
		std::string strByte = strFeaturesByte.substr(pos, 2); //每个文本字节

		//cout << strByte <<"/" << endl;

		if (strByte == "??") //判断是不是??
		{
			upFuzzyBytesBuf[nFeaturesBytePos] = '\x00';
		}
		else 	if (strByte.substr(0, 1) == "?")
		{
			upFuzzyBytesBuf[nFeaturesBytePos] = '\x01';
		}
		else 	if (strByte.substr(1, 2) == "?")
		{
			upFuzzyBytesBuf[nFeaturesBytePos] = '\x10';
		}
		else
		{
			upFuzzyBytesBuf[nFeaturesBytePos] = '\x11';
		}

		//把?替换成0
		replace_all_distinct(strByte, "?", "0");
		//cout << strByte << endl;
		int nByte = 0;
		std::stringstream ssConvertBuf;
		ssConvertBuf.str(strByte);
		ssConvertBuf >> std::hex >> nByte; //十六进制字节文本转成十进制


		upFeaturesBytesBuf[nFeaturesBytePos] = nByte; //将十进制数值写进储存起来
		nFeaturesBytePos++;

	}
	return SearchMemoryBytesThread2(
		pDriver,
		hProcess,
		vScanMemMaps,
		upFeaturesBytesBuf.get(),
		strFeaturesByte.length() / 2,
		upFuzzyBytesBuf.get(),
		nThreadCount,
		nFastScanAlignment,
		vOutputAddr,
		nSearchProgress,
		nFoundAddrCount);
}

/*
内存搜索（搜索内存特征码），成功返回true，失败返回false
hProcess：被搜索的进程句柄
vScanMemMaps: 被搜索的进程内存区域
vFeaturesByte：字节特征码，如“char vBytes[] = {'\x68', '\x00', '\x00', '\x00', '\x40', '\x?3', '\x??', '\x7?', '\x??', '\x??', '\x??', '\x??', '\x??', '\x50', '\xE8};”
nFeaturesByteLen：字节特征码长度
vFuzzyBytes：模糊字节，不变动的位置用1表示，变动的位置用0表示，如“char vFuzzy[] = {'\x11', '\x11', '\x11', '\x11', '\x11', '\x01', '\x00', '\x10', '\x00', '\x00', '\x00', '\x00', '\x00', '\x11', '\x11};”
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
static void SearchMemoryBytesThread2(CMemoryReaderWriter *pDriver, uint64_t hProcess, std::vector<MEM_SECTION_INFO>vScanMemMaps, char vFeaturesByte[], size_t nFeaturesByteLen, char vFuzzyBytes[], int nThreadCount, size_t nFastScanAlignment, std::vector<ADDR_RESULT_INFO> * vOutputAddr, std::atomic<int> *nSearchProgress, std::atomic<uint64_t> *nFoundAddrCount)
{
	//获取当前系统内存大小
	struct sysinfo si;
	sysinfo(&si);
	size_t nMaxMemSize = si.totalram;

	//生成特征码容错文本
	std::string strFuzzyCode = ""; //如：xxxxx????????xx
	for (size_t i = 0; i < nFeaturesByteLen; i++)
	{
		if (vFuzzyBytes[i] == '\x00')
		{
			strFuzzyCode += "??";
		}
		else if (vFuzzyBytes[i] == '\x01')
		{
			strFuzzyCode += "?x";
		}
		else if (vFuzzyBytes[i] == '\x10')
		{
			strFuzzyCode += "x?";
		}
		else
		{
			strFuzzyCode += "xx";
		}
	}
	std::mutex mtxResultAccessLock;			//搜索结果汇总数组访问锁
	std::vector<ADDR_RESULT_INFO> vResultAddr;		//全部线程的搜索结果汇总数组

	//分配线程进行内存搜索
	size_t nScanMemMapsMaxCount = vScanMemMaps.size(); //记录被搜索的进程内存区域总数
	size_t nThreadJob = nScanMemMapsMaxCount / nThreadCount; //每条线程需要处理的内存区段任务数
	//cout << "每 " << nThreadJob << endl;
	if (nThreadJob == 0)
	{
		nThreadJob = 1;
		nThreadCount = nScanMemMapsMaxCount;
	}
	std::atomic<int> nWorkingThreadCount; //记录实时正在工作的搜索线程数
	std::atomic<uint64_t> nFinishedMemSectionJobCount; //记录实时已处理完成的内存区段任务总数
	nWorkingThreadCount = nThreadCount;
	nFinishedMemSectionJobCount = 0;

	//开始分配线程搜索内存
	for (int i = 0; i < nThreadCount; i++)
	{
		//每条线程需要处理的内存区段任务数组
		std::unique_ptr<std::vector<MEM_SECTION_INFO>> upVecThreadJob = std::make_unique<std::vector<MEM_SECTION_INFO>>();

		for (size_t count = 0; count < nThreadJob; count++)
		{
			MEM_SECTION_INFO task = *vScanMemMaps.rbegin(); //从总内存区段任务队列末尾取出一个内存区段任务
			upVecThreadJob->push_back(task); //取完保存起来
			vScanMemMaps.pop_back(); //取完末尾后删除
		}

		if (i + 1 == nThreadCount)
		{
			//如果是最后一条线程了，则需要把剩下的内存区段任务全部处理
			for (MEM_SECTION_INFO task : vScanMemMaps)
			{
				upVecThreadJob->push_back(task);
			}
		}


		//内存搜索线程
		std::unique_ptr<unsigned char[]> vCopyFeaturesByte = std::make_unique<unsigned char[]>(nFeaturesByteLen);
		memcpy(&vCopyFeaturesByte[0], &vFeaturesByte[0], nFeaturesByteLen); //每条线程都传一份拷贝的特征码进去

		//内存搜索线程
		std::thread td(
			[](
				CMemoryReaderWriter *pDriver,
				size_t nMaxMallocMem, //最大申请内存字节
				std::unique_ptr<std::vector<MEM_SECTION_INFO>> upVecThreadJob, //每条线程需要处理的内存区段任务数组
				uint64_t hProcess, //被搜索进程句柄
				std::atomic<int> *nWorkingThreadCount, //记录实时正在工作的搜索线程数
				std::atomic<uint64_t> *nFinishedMemSectionJobCount, //记录实时已处理完成的内存区段任务总数
				std::atomic<uint64_t> *nFoundAddrCount, //记录实时已找到的地址数目
				std::unique_ptr<unsigned char[]>vFeaturesByte,	//特征码字节数组
				std::unique_ptr<std::string>strFuzzyCode,	//特征码容错文本
				std::mutex *mtxResultAccessLock,	//搜索结果汇总数组访问锁
				std::vector<ADDR_RESULT_INFO> *pvResultAddr,	//全部线程的搜索结果汇总数组
				size_t nFastScanAlignment	//快速扫描的对齐位数
				)->void
		{
			//cout << "我 " << upVecThreadJob->size() << endl;

			//是否启用特征码容错搜索
			int isSimpleSearch = (strFuzzyCode->find("?") == -1) ? 1 : 0;

			std::vector<ADDR_RESULT_INFO> vThreadOutput; //存放当前线程的搜索结果

			for (MEM_SECTION_INFO memSecInfo : *upVecThreadJob) //线程的每个任务
			{
				//当前内存区域块大小超过了最大内存申请的限制，所以跳过
				if (memSecInfo.nSectionSize > nMaxMallocMem) { continue; }
				unsigned char *pnew = new (std::nothrow) unsigned char[memSecInfo.nSectionSize];
				if (!pnew) { /*cout << "malloc "<< memSecInfo.nSectionSize  << " failed."<< endl;*/ continue; }
				memset(pnew, 0, memSecInfo.nSectionSize);

				std::shared_ptr<unsigned char> spMemBuf(pnew, std::default_delete<unsigned char[]>());
				size_t dwRead = 0;
				if (!pDriver->ReadProcessMemory(hProcess, memSecInfo.npSectionAddr, spMemBuf.get(), memSecInfo.nSectionSize, &dwRead, FALSE))
				{
					continue;
				}

				//寻找字节集
				std::vector<size_t>vFindAddr;
				if (isSimpleSearch)
				{
					//不需要容错搜索
					FindBytes((size_t)spMemBuf.get(), dwRead, &vFeaturesByte[0], strFuzzyCode->length() / 2, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
				}
				else
				{
					//需要容错搜索
					FindFeaturesBytes((size_t)spMemBuf.get(), dwRead, &vFeaturesByte[0], strFuzzyCode->c_str(), strFuzzyCode->length() / 2, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
				}

				for (size_t addr : vFindAddr)
				{
					//保存搜索结果
					ADDR_RESULT_INFO aInfo;
					aInfo.addr = (uint64_t)((uint64_t)memSecInfo.npSectionAddr + (uint64_t)addr - (uint64_t)spMemBuf.get());
					aInfo.size = strFuzzyCode->length() / 2;
					std::shared_ptr<unsigned char> sp(new unsigned char[aInfo.size], std::default_delete<unsigned char[]>());
					memcpy(sp.get(), (void*)addr, aInfo.size);
					aInfo.spSaveData = sp;
					vThreadOutput.push_back(aInfo);
				}
				(*nFinishedMemSectionJobCount)++; //实时已处理完成的内存区段任务总数+1
			}

			//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
			mtxResultAccessLock->lock();
			for (ADDR_RESULT_INFO newAddr : vThreadOutput)
			{
				pvResultAddr->push_back(newAddr);
			}
			mtxResultAccessLock->unlock();
			(*nWorkingThreadCount)--; //实时正在工作的搜索线程数-1
			return;
		},
			pDriver,
			nMaxMemSize,
			std::move(upVecThreadJob),
			hProcess,
			&nWorkingThreadCount,
			&nFinishedMemSectionJobCount,
			nFoundAddrCount,
			std::move(vCopyFeaturesByte),
			std::make_unique<std::string>(strFuzzyCode),
			&mtxResultAccessLock,
			&vResultAddr,
			nFastScanAlignment
			);
		td.detach();
	}

	//等待所有搜索线程结束汇总
	while (nWorkingThreadCount != 0)
	{
		//计算搜索进度为
		uint64_t progress = ((double)nFinishedMemSectionJobCount / (double)nScanMemMapsMaxCount) * 100;
		if (progress >= 100)
		{
			(*nSearchProgress) = 99;
		}
		else
		{
			(*nSearchProgress) = progress;
		}

		sleep(0);
	}

	vOutputAddr->clear();
	vOutputAddr->assign(vResultAddr.begin(), vResultAddr.end());
	sort(vOutputAddr->begin(), vOutputAddr->end(), [](ADDR_RESULT_INFO a, ADDR_RESULT_INFO b) -> bool { return a.addr < b.addr; });

	//设置搜索进度为100
	(*nSearchProgress) = 100;
	return;
}


/*
再次搜索内存（搜索文本特征码），成功返回true，失败返回false
hProcess：被搜索的进程句柄
vScanMemAddrList: 需要再次搜索的内存地址列表
strFeaturesByte: 十六进制的字节特征码，搜索规则与OD相同，如“68 00 00 00 40 ?3 7? ?? ?? ?? ?? ?? ?? 50 E8”
nThreadCount: 用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
static void SearchNextMemoryBytesThread(CMemoryReaderWriter *pDriver, uint64_t hProcess, std::vector<ADDR_RESULT_INFO>vScanMemAddrList, std::string strFeaturesByte, int nThreadCount, size_t nFastScanAlignment, std::vector<ADDR_RESULT_INFO> * vOutputAddr, std::atomic<int> *nSearchProgress, std::atomic<uint64_t> *nFoundAddrCount)
{


	//预处理特征码
	replace_all_distinct(strFeaturesByte, " ", "");//去除空格
	if (strFeaturesByte.length() % 2)
	{
		//设置搜索进度为100
		(*nSearchProgress) = 100;
		return;
	}

	//特征码字节数组
	std::unique_ptr<char[]> upFeaturesBytesBuf = std::make_unique<char[]>(strFeaturesByte.length() / 2); //如：68 00 00 00 40 ?3 7? ?? ?? ?? ?? ?? ?? 50 E8

	//特征码容错数组
	std::unique_ptr<char[]> upFuzzyBytesBuf = std::make_unique<char[]>(strFeaturesByte.length() / 2);

	size_t nFeaturesBytePos = 0; //记录写入特征码字节数组的位置

	//遍历每个文本字节
	for (std::string::size_type pos(0); pos < strFeaturesByte.length(); pos += 2)
	{
		std::string strByte = strFeaturesByte.substr(pos, 2); //每个文本字节

		//cout << strByte <<"/" << endl;

		if (strByte == "??") //判断是不是??
		{
			upFuzzyBytesBuf[nFeaturesBytePos] = '\x00';
		}
		else 	if (strByte.substr(0, 1) == "?")
		{
			upFuzzyBytesBuf[nFeaturesBytePos] = '\x01';
		}
		else 	if (strByte.substr(1, 2) == "?")
		{
			upFuzzyBytesBuf[nFeaturesBytePos] = '\x10';
		}
		else
		{
			upFuzzyBytesBuf[nFeaturesBytePos] = '\x11';
		}

		//把?替换成0
		replace_all_distinct(strByte, "?", "0");
		//cout << strByte << endl;
		int nByte = 0;
		std::stringstream ssConvertBuf;
		ssConvertBuf.str(strByte);
		ssConvertBuf >> std::hex >> nByte; //十六进制字节文本转成十进制


		upFeaturesBytesBuf[nFeaturesBytePos] = nByte; //将十进制数值写进储存起来
		nFeaturesBytePos++;

	}
	return SearchNextMemoryBytesThread2(
		pDriver,
		hProcess,
		vScanMemAddrList,
		upFeaturesBytesBuf.get(),
		strFeaturesByte.length() / 2,
		upFuzzyBytesBuf.get(),
		nThreadCount,
		nFastScanAlignment,
		vOutputAddr,
		nSearchProgress,
		nFoundAddrCount);

}

/*
再次搜索内存（搜索内存特征码），成功返回true，失败返回false
hProcess：被搜索的进程句柄
vScanMemAddrList: 需要再次搜索的内存地址列表
vFeaturesByte：字节特征码，如“char vBytes[] = {0x68, 0x00, 0x00, 0x00, 0x40, 0x?3, 0x??, 0x7?, 0x??, 0x??, 0x??, 0x??, 0x??, 0x50, 0xE8};”
nFeaturesByteLen：字节特征码长度
vFuzzyBytes：模糊字节，不变动的位置用1表示，变动的位置用0表示，如“char vFuzzy[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11};”
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
static void SearchNextMemoryBytesThread2(CMemoryReaderWriter *pDriver, uint64_t hProcess, std::vector<ADDR_RESULT_INFO>vScanMemAddrList, char vFeaturesByte[], size_t nFeaturesByteLen, char vFuzzyBytes[], int nThreadCount, size_t nFastScanAlignment, std::vector<ADDR_RESULT_INFO> * vOutputAddr, std::atomic<int> *nSearchProgress, std::atomic<uint64_t> *nFoundAddrCount)
{

	//生成特征码容错文本
	std::string strFuzzyCode = ""; //如：xxxxx????????xx
	for (size_t i = 0; i < nFeaturesByteLen; i++)
	{
		if (vFuzzyBytes[i] == '\x00')
		{
			strFuzzyCode += "??";
		}
		else if (vFuzzyBytes[i] == '\x01')
		{
			strFuzzyCode += "?x";
		}
		else if (vFuzzyBytes[i] == '\x10')
		{
			strFuzzyCode += "x?";
		}
		else
		{
			strFuzzyCode += "xx";
		}
	}


	std::mutex mtxResultAccessLock;			//搜索结果汇总数组访问锁
	std::vector<ADDR_RESULT_INFO> vResultAddr;		//全部线程的搜索结果汇总数组

	//分配线程进行内存搜索
	size_t nScanMemAddrMaxCount = vScanMemAddrList.size(); //记录需要再次搜索的内存地址总数
	size_t nThreadJob = nScanMemAddrMaxCount / nThreadCount; //每条线程需要处理的内存地址任务数
	//cout << "每 " << nThreadJob << endl;
	if (nThreadJob == 0)
	{
		nThreadJob = 1;
		nThreadCount = nScanMemAddrMaxCount;
	}

	std::atomic<int> nWorkingThreadCount; //记录实时正在工作的搜索线程数
	std::atomic<uint64_t> nFinishedMemAddrJobCount{0}; //记录实时已处理完成的内存地址任务总数
	nWorkingThreadCount = nThreadCount;
	

	//开始分配线程搜索内存
	for (int i = 0; i < nThreadCount; i++)
	{
		//每条线程需要处理的内存区段任务数组
		std::unique_ptr<std::vector<ADDR_RESULT_INFO>> upVecThreadJob = std::make_unique<std::vector<ADDR_RESULT_INFO>>();

		for (size_t count = 0; count < nThreadJob; count++)
		{
			ADDR_RESULT_INFO task = *vScanMemAddrList.rbegin(); //从总内存区段任务队列末尾取出一个内存区段任务
			upVecThreadJob->push_back(task); //取完保存起来
			vScanMemAddrList.pop_back(); //取完末尾后删除
		}

		if (i + 1 == nThreadCount)
		{
			//如果是最后一条线程了，则需要把剩下的内存地址任务全部处理
			for (ADDR_RESULT_INFO task : vScanMemAddrList)
			{
				upVecThreadJob->push_back(task);
			}
		}


		//内存搜索线程
		std::unique_ptr<unsigned char[]> vCopyFeaturesByte = std::make_unique<unsigned char[]>(nFeaturesByteLen);
		memcpy(&vCopyFeaturesByte[0], &vFeaturesByte[0], nFeaturesByteLen); //每条线程都传一份拷贝的特征码进去

		//内存搜索线程
		std::thread td(
			[](
				CMemoryReaderWriter *pDriver,
				std::unique_ptr<std::vector<ADDR_RESULT_INFO>> upVecThreadJob, //每条线程需要处理的内存地址任务数组
				uint64_t hProcess, //被搜索进程句柄
				std::atomic<int> *nWorkingThreadCount, //记录实时正在工作的搜索线程数
				std::atomic<uint64_t> *nFinishedMemSectionJobCount, //记录实时已处理完成的内存区段任务总数
				std::atomic<uint64_t> *nFoundAddrCount, //记录实时已找到的地址数目
				std::unique_ptr<unsigned char[]>vFeaturesByte,	//特征码字节数组
				std::unique_ptr<std::string>strFuzzyCode,	//特征码容错文本
				std::mutex *mtxResultAccessLock,	//搜索结果汇总数组访问锁
				std::vector<ADDR_RESULT_INFO> *pvResultAddr,	//全部线程的搜索结果汇总数组
				size_t nFastScanAlignment	//快速扫描的对齐位数
				)->void
		{
			//cout << "我 " << upVecThreadJob->size() << endl;

			//是否启用特征码容错搜索
			int isSimpleSearch = (strFuzzyCode->find("?") == -1) ? 1 : 0;


			std::vector<ADDR_RESULT_INFO> vThreadOutput; //存放当前线程的搜索结果

			for (ADDR_RESULT_INFO memAddrJob : *upVecThreadJob) //线程的每个任务
			{
				(*nFinishedMemSectionJobCount)++; //实时已处理完成的内存区段任务总数+1

				size_t memBufSize = strFuzzyCode->length() / 2;
				unsigned char *pnew = new (std::nothrow) unsigned char[memBufSize];
				if (!pnew) { /*cout << "malloc "<< memSecInfo.nSectionSize  << " failed."<< endl;*/ continue; }
				memset(pnew, 0, memBufSize);

				std::shared_ptr<unsigned char> spMemBuf(pnew, std::default_delete<unsigned char[]>());
				size_t dwRead = 0;
				if (!pDriver->ReadProcessMemory(hProcess, memAddrJob.addr, spMemBuf.get(), memBufSize, &dwRead, FALSE))
				{
					continue;
				}
				if (dwRead != memBufSize)
				{
					continue;
				}
				//寻找字节集
				std::vector<size_t>vFindAddr;


				if (isSimpleSearch)
				{
					//不需要容错搜索
					FindBytes((size_t)spMemBuf.get(), dwRead, &vFeaturesByte[0], strFuzzyCode->length() / 2, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
				}
				else
				{
					//需要容错搜索
					FindFeaturesBytes((size_t)spMemBuf.get(), dwRead, &vFeaturesByte[0], strFuzzyCode->c_str(), strFuzzyCode->length() / 2, nFastScanAlignment, vFindAddr, *nFoundAddrCount);
				}

				if (vFindAddr.size())
				{
					//保存搜索结果
					memcpy(memAddrJob.spSaveData.get(), spMemBuf.get(), memBufSize);
					vThreadOutput.push_back(memAddrJob);
					//记录实时找到的地址数
					(*nFoundAddrCount)++;
				}

			}
			//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
			mtxResultAccessLock->lock();
			for (ADDR_RESULT_INFO newAddr : vThreadOutput)
			{
				pvResultAddr->push_back(newAddr);
			}
			mtxResultAccessLock->unlock();
			(*nWorkingThreadCount)--; //实时正在工作的搜索线程数-1
			return;

		},
			pDriver,
			std::move(upVecThreadJob),
			hProcess,
			&nWorkingThreadCount,
			&nFinishedMemAddrJobCount,
			nFoundAddrCount,
			std::move(vCopyFeaturesByte),
			std::make_unique<std::string>(strFuzzyCode),
			&mtxResultAccessLock,
			&vResultAddr,
			nFastScanAlignment
			);
		td.detach();

	}

	//等待所有搜索线程结束汇总
	while (nWorkingThreadCount != 0)
	{
		//计算搜索进度为
		uint64_t progress = ((double)nFinishedMemAddrJobCount / (double)nScanMemAddrMaxCount) * 100;
		if (progress >= 100)
		{
			(*nSearchProgress) = 99;
		}
		else
		{
			(*nSearchProgress) = progress;
		}

		sleep(0);
	}

	vOutputAddr->clear();
	vOutputAddr->assign(vResultAddr.begin(), vResultAddr.end());
	sort(vOutputAddr->begin(), vOutputAddr->end(), [](ADDR_RESULT_INFO a, ADDR_RESULT_INFO b) -> bool { return a.addr < b.addr; });

	//设置搜索进度为100
	(*nSearchProgress) = 100;
	return;
}

/*
拷贝进程内存数据，成功返回true，失败返回false
hProcess：被搜索的进程句柄
vScanMemMaps: 被拷贝的进程内存区域
vOutputMemCopy：拷贝进程内存数据的存放区域
scanValueType：记录本次拷贝进程内存数据的值类型，1:1字节，2:2字节，4:4字节，8:8字节，9:单浮点，10:双浮点
nSearchProgress：记录实时搜索百分比进度
nFoundAddrCount：记录实时已找到的地址数目
*/
static void CopyProcessMemDataThread(CMemoryReaderWriter *pDriver, uint64_t hProcess, std::vector<MEM_SECTION_INFO>vScanMemMaps, std::vector<COPY_MEM_INFO> * vOutputMemCopy, int scanValueType, std::atomic<int> *nSearchProgress, std::atomic<uint64_t> *nFoundAddrCount)
{

	//获取当前系统内存大小
	struct sysinfo si;
	sysinfo(&si);
	size_t nMaxMemSize = si.totalram;

	size_t  nMemRegionCount = vScanMemMaps.size();

	for (int i = 0; i < nMemRegionCount; ++i) {

		MEM_SECTION_INFO memSecInfo = vScanMemMaps.at(i);

		//当前内存区域块大小超过了最大内存申请的限制，所以跳过
		if (memSecInfo.nSectionSize > nMaxMemSize) { continue; }

		unsigned char * pnew = new (std::nothrow) unsigned char[memSecInfo.nSectionSize];
		if (!pnew) { /*cout << "malloc "<< memSecInfo.nSectionSize  << " failed."<< endl;*/ continue; }
		memset(pnew, 0, memSecInfo.nSectionSize);


		std::shared_ptr<unsigned char>spMemBuf(pnew, std::default_delete<unsigned char[]>());
		size_t dwRead = 0;
		if (!pDriver->ReadProcessMemory(hProcess, memSecInfo.npSectionAddr, spMemBuf.get(), memSecInfo.nSectionSize, &dwRead, FALSE))
		{
			continue;
		}
		COPY_MEM_INFO copyMem = { 0 };
		copyMem.npSectionAddr = memSecInfo.npSectionAddr;
		copyMem.nSectionSize = dwRead;
		copyMem.spSaveMemBuf = spMemBuf;
		(*vOutputMemCopy).push_back(copyMem);

		switch (scanValueType)
		{
		case 1: //字节搜索
			(*nFoundAddrCount) += dwRead;
			break;
		case 2: //2 字节搜索
			(*nFoundAddrCount) += ((double)dwRead / (double)2);
			break;
		case 4: //4 字节搜索
			(*nFoundAddrCount) += ((double)dwRead / (double)4);
			break;
		case 8: //8 字节搜索
			(*nFoundAddrCount) += ((double)dwRead / (double)8);
			break;
		case 9: //单浮点搜索
			(*nFoundAddrCount) += ((double)dwRead / (double)4);
			break;
		case 10: //双浮点搜索
			(*nFoundAddrCount) += ((double)dwRead / (double)8);
			break;
		default:
			break;
		}


		//计算搜索进度为
		uint64_t progress = ((double)i / (double)nMemRegionCount) * 100;
		if (progress >= 100)
		{
			(*nSearchProgress) = 99;
		}
		else
		{
			(*nSearchProgress) = progress;
		}
	}

	(*nSearchProgress) = 100;

	return;
}


/*
寻找精确数值
使用方法:
dwAddr：要搜索的缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索的数值
errorRange: 误差范围（当搜索的数值为float或者double时此值生效）
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindValue(size_t dwAddr, size_t dwLen, T value, float errorRange, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount)
{
	if (typeid(T) == typeid(static_cast<float>(0)) || typeid(T) == typeid(static_cast<double>(0)))
	{
		FindBetween(dwAddr, dwLen, value - errorRange, value + errorRange, nFastScanAlignment, vOutputAddr, nFoundAddrCount);
	}
	else
	{
		vOutputAddr.clear();

		for (size_t i = 0; i < dwLen; i += nFastScanAlignment)
		{
			if ((dwLen - i) < sizeof(T))
			{
				//要搜索的数据已经小于T的长度了
				break;
			}

			T* pData = (T*)(dwAddr + i);

			if (*pData == value)
			{
				vOutputAddr.push_back((size_t)((size_t)dwAddr + (size_t)i));

				//记录实时找到的地址数
				nFoundAddrCount++;

			}
			continue;
		}
	}
	return;

}

/*
寻找大于的数值
使用方法:
dwAddr：要搜索的缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索大于的数值
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindMax(size_t dwAddr, size_t dwLen, T value, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount)
{
	vOutputAddr.clear();

	for (size_t i = 0; i < dwLen; i += nFastScanAlignment)
	{
		if ((dwLen - i) < sizeof(T))
		{
			//要搜索的数据已经小于T的长度了
			break;
		}

		T* pData = (T*)(dwAddr + i);
		if (*pData > value)
		{
			vOutputAddr.push_back((size_t)((size_t)dwAddr + (size_t)i));

			//记录实时找到的地址数
			nFoundAddrCount++;
		}
		continue;
	}
	return;
}



/*
寻找小于的数值
使用方法:
dwAddr：要搜索的缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索小于的char数值
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindMin(size_t dwAddr, size_t dwLen, T value, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount)
{
	vOutputAddr.clear();

	for (size_t i = 0; i < dwLen; i += nFastScanAlignment)
	{
		if ((dwLen - i) < sizeof(T))
		{
			//要搜索的数据已经小于T的长度了
			break;
		}

		T* pData = (T*)(dwAddr + i);
		if (*pData < value)
		{
			vOutputAddr.push_back((size_t)((size_t)dwAddr + (size_t)i));

			//记录实时找到的地址数
			nFoundAddrCount++;
		}
		continue;
	}
	return;
}
/*
寻找两者之间的数值
使用方法:
dwAddr：要搜索的缓冲区地址
dwLen：要搜索的缓冲区大小
value1: 要搜索的数值1
value2: 要搜索的数值2
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindBetween(size_t dwAddr, size_t dwLen, T value1, T value2, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount)
{
	vOutputAddr.clear();

	for (size_t i = 0; i < dwLen; i += nFastScanAlignment)
	{
		if ((dwLen - i) < sizeof(T))
		{
			//要搜索的数据已经小于T的长度了
			break;
		}

		T* pData = (T*)(dwAddr + i);
		T cData = *pData;

		if (cData > value1 && cData < value2)
		{
			vOutputAddr.push_back((size_t)((size_t)dwAddr + (size_t)i));

			//记录实时找到的地址数
			nFoundAddrCount++;
		}
		continue;
	}
	return;
}


/*
寻找增加的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindUnknowAdd(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount)
{
	vOutputAddr.clear();

	for (size_t i = 0; i < dwLen; i += nFastScanAlignment)
	{
		if ((dwLen - i) < sizeof(T))
		{
			//要搜索的数据已经小于T的长度了
			break;
		}

		T* pOldData = (T*)(dwOldAddr + i);
		T* pNewData = (T*)(dwNewAddr + i);
		if (*pNewData > *pOldData)
		{
			vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));

			//记录实时找到的地址数
			nFoundAddrCount++;
		}
		continue;
	}
	return;
}
/*
寻找增加了已知值的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索增加的已知值
errorRange: 误差范围（当搜索的数值为float或者double时此值生效）
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindAdd(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, T value, float errorRange, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount)
{
	vOutputAddr.clear();

	float value1, value2;
	if (typeid(T) == typeid(static_cast<float>(0)) || typeid(T) == typeid(static_cast<double>(0)))
	{
		value1 = value - errorRange;
		value2 = value + errorRange;
	}
	for (size_t i = 0; i < dwLen; i += nFastScanAlignment)
	{
		if ((dwLen - i) < sizeof(T))
		{
			//要搜索的数据已经小于T的长度了
			break;
		}

		T* pOldData = (T*)(dwOldAddr + i);
		T* pNewData = (T*)(dwNewAddr + i);
		T myAdd = ((*pNewData) - (*pOldData));

		if (typeid(T) == typeid(static_cast<float>(0)) || typeid(T) == typeid(static_cast<double>(0)))
		{
			if (myAdd >= value1 && myAdd <= value2)
			{
				vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));
				//记录实时找到的地址数
				nFoundAddrCount++;

			}
		}
		else
		{
			if (myAdd == value)
			{
				vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));
				//记录实时找到的地址数
				nFoundAddrCount++;

			}
		}
			
		continue;
	}
	return;
}
/*
寻找减少的unsigned char的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindUnknowSum(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount)
{
	vOutputAddr.clear();

	for (size_t i = 0; i < dwLen; i += nFastScanAlignment)
	{
		if ((dwLen - i) < sizeof(T))
		{
			//要搜索的数据已经小于T的长度了
			break;
		}

		T* pOldData = (T*)(dwOldAddr + i);
		T* pNewData = (T*)(dwNewAddr + i);
		if (*pNewData < *pOldData)
		{
			vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));

			//记录实时找到的地址数
			nFoundAddrCount++;
		}
		continue;
	}
	return;
}
/*
寻找减少了已知值的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索减少的已知值
errorRange: 误差范围（当搜索的数值为float或者double时此值生效）
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindSum(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, T value, float errorRange, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount)
{
	vOutputAddr.clear();

	float value1, value2;
	if (typeid(T) == typeid(static_cast<float>(0)) || typeid(T) == typeid(static_cast<double>(0)))
	{
		value1 = value - errorRange;
		value2 = value + errorRange;
	}

	for (size_t i = 0; i < dwLen; i += nFastScanAlignment)
	{
		if ((dwLen - i) < sizeof(T))
		{
			//要搜索的数据已经小于T的长度了
			break;
		}

		T* pOldData = (T*)(dwOldAddr + i);
		T* pNewData = (T*)(dwNewAddr + i);
		T mySum = ((*pOldData) - (*pNewData));

		if (typeid(T) == typeid(static_cast<float>(0)) || typeid(T) == typeid(static_cast<double>(0)))
		{
			if (mySum >= value1 && mySum <= value2)
			{
				vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));

				//记录实时找到的地址数
				nFoundAddrCount++;
			}
		}
		else
		{
			if (mySum == value)
			{
				vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));

				//记录实时找到的地址数
				nFoundAddrCount++;
			}
		}
		
			
		continue;
	}
	return;
}
/*
寻找变动的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindChanged(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount)
{
	vOutputAddr.clear();

	for (size_t i = 0; i < dwLen; i += nFastScanAlignment)
	{
		if ((dwLen - i) < sizeof(T))
		{
			//要搜索的数据已经小于T的长度了
			break;
		}

		T* pOldData = (T*)(dwOldAddr + i);
		T* pNewData = (T*)(dwNewAddr + i);
		if (*pNewData != *pOldData)
		{
			vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));

			//记录实时找到的地址数
			nFoundAddrCount++;
		}
		continue;
	}
	return;
}
/*
寻找未变动的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
nFoundAddrCount：记录实时找到的地址数目
*/
template<typename T> static inline void FindNoChange(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount)
{
	vOutputAddr.clear();

	for (size_t i = 0; i < dwLen; i += nFastScanAlignment)
	{
		if ((dwLen - i) < sizeof(T))
		{
			//要搜索的数据已经小于T的长度了
			break;
		}

		T* pOldData = (T*)(dwOldAddr + i);
		T* pNewData = (T*)(dwNewAddr + i);
		if (*pNewData == *pOldData)
		{
			vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));

			//记录实时找到的地址数
			nFoundAddrCount++;
		}
		continue;
	}
	return;
}




/*
寻找字节集
dwAddr: 要搜索的缓冲区地址
dwLen: 要搜索的缓冲区长度
bMask: 要搜索的字节集
szMask: 要搜索的字节集模糊码（xx=已确定字节，??=可变化字节，支持x?、?x）
nMaskLen: 要搜索的字节集长度
nFastScanAlignment: 为快速扫描的对齐位数，CE默认为1
vOutputAddr: 搜索出来的结果地址存放数组
nFoundAddrCount: 实时找到的地址数量
使用方法：
std::vector<size_t> vAddr;
std::atomic<uint64_t> nFoundAddrCount;
nFoundAddrCount = 0;
FindFeaturesBytes((size_t)lpBuffer, dwBufferSize, (PBYTE)"\x8B\xE8\x00\x00\x00\x00\x33\xC0\xC7\x06\x00\x00\x00\x00\x89\x86\x40", "xxxx??xx?xx?xxxxx",17,1,vAddr , nFoundAddrCount);
*/
static inline void FindFeaturesBytes(size_t dwAddr, size_t dwLen, unsigned char *bMask, const char* szMask, size_t nMaskLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount)
{
	vOutputAddr.clear();
	for (size_t i = 0; i < dwLen; i += nFastScanAlignment)
	{
		if ((dwLen - i) < nMaskLen)
		{
			//要搜索的数据已经小于特征码的长度了
			break;
		}
		unsigned char* pData = (unsigned char*)(dwAddr + i);
		unsigned char*bTemMask = bMask;
		const char* szTemMask = szMask;

		bool bContinue = false;
		for (; *szTemMask; szTemMask += 2, ++pData, ++bTemMask)
		{
			if ((*szTemMask == 'x') && (*(szTemMask + 1) == 'x') && ((*pData) != (*bTemMask)))
			{
				bContinue = true;
				break;
			}
			else if ((*szTemMask == 'x') && (*(szTemMask + 1) == '?') && ((((*pData) >> 4) & 0xFu) != (((*bTemMask) >> 4) & 0xFu)))
			{
				bContinue = true;
				break;
			}
			else if ((*szTemMask == '?') && (*(szTemMask + 1) == 'x') && (((*pData) & 0xFu) != ((*bTemMask) & 0xFu)))
			{
				bContinue = true;
				break;
			}
		}
		if (bContinue)
		{
			continue;
		}

		if ((*szTemMask) == '\x00')
		{
			vOutputAddr.push_back((size_t)((size_t)dwAddr + (size_t)i));

			//记录实时找到的地址数
			nFoundAddrCount++;
		}
	}
	return;
}
/*
寻找字节集
dwWaitSearchAddress: 要搜索的缓冲区地址
dwLen: 要搜索的缓冲区长度
bForSearch: 要搜索的字节集
ifLen: 要搜索的字节集长度
nFastScanAlignment: 为快速扫描的对齐位数，CE默认为1
vOutputAddr: 搜索出来的结果地址存放数组
nFoundAddrCount: 实时找到的地址数量
使用方法：
std::vector<size_t> vAddr;
std::atomic<uint64_t> nFoundAddrCount;
nFoundAddrCount = 0;
FindFeaturesBytes((size_t)lpBuffer, dwBufferSize, (PBYTE)"\x8B\xE8\x00\x00\x00\x00\x33\xC0\xC7\x06\x00\x00\x00\x00\x89\x86\x40", 17,1,vAddr , nFoundAddrCount);
*/
static inline void FindBytes(size_t dwWaitSearchAddress, size_t dwLen, unsigned char *bForSearch, size_t ifLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr, std::atomic<uint64_t> &nFoundAddrCount)
{
	for (size_t i = 0; i < dwLen; i += nFastScanAlignment)
	{
		if ((dwLen - i) < ifLen)
		{
			//要搜索的数据已经小于特征码的长度了
			break;
		}
		unsigned char* pData = (unsigned char*)(dwWaitSearchAddress + i);
		unsigned char*bTemForSearch = bForSearch;

		bool bContinue = false;
		for (size_t y = 0; y < ifLen; y++, ++pData, ++bTemForSearch)
		{
			if (*pData != *bTemForSearch)
			{
				bContinue = true;
				break;
			}
		}
		if (bContinue)
		{
			continue;
		}
		vOutputAddr.push_back((size_t)((size_t)dwWaitSearchAddress + (size_t)i));
		//记录实时找到的地址数
		nFoundAddrCount++;
	}
	return;
}

static inline std::string& replace_all_distinct(std::string& str, const std::string& old_value, const std::string& new_value)
{
	for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length())
	{
		if ((pos = str.find(old_value, pos)) != std::string::npos)
		{
			str.replace(pos, old_value.length(), new_value);
		}
		else
		{
			break;
		}
	}
	return str;
}

#endif /* MEM_SEARCH_HELPER_H_ */

