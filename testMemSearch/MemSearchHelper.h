#ifndef MEM_SEARCH_HELPER_H_
#define MEM_SEARCH_HELPER_H_
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <mutex>
#include <thread>
#include <sstream>
#include <functional>
#ifdef __linux__
#include <unistd.h>
#include <sys/sysinfo.h>
#endif

#include "../testKo/MemoryReaderWriter37.h"
#include "MemReaderWriterProxy.h"
#include "SafeVector.h"

#define GET_JOB_COUNT 3

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

template<typename T> struct BATCH_BETWEEN_VAL
{
	T val1 = {}; //批量搜索值在两值之间的值1
	T val2 = {}; //批量搜索值在两值之间的值2
	union
	{
		char chCtx;
		short sCtx;
		int nCtx;
		long lCtx;
		unsigned long ulCtx;
		unsigned long long ullCtx;
		uint16_t u16Ctx;
		uint32_t u32Ctx;
		uint64_t u64Ctx;
		int16_t i16Ctx;
		int32_t i32Ctx;
		int64_t i64Ctx = 0;
	} markContext;  //附带的上下文，可传递消息
};
template<typename T> struct BATCH_BETWEEN_VAL_ADDR_RESULT
{
	ADDR_RESULT_INFO addrInfo; //结果地址
	BATCH_BETWEEN_VAL<T> originalCondition; //原始搜索条件
};

enum SCAN_TYPE
{
	ACCURATE_VAL = 0,	//精确数值
	LARGER_THAN_VAL,	//值大于
	LESS_THAN_VAL,		//值小于
	BETWEEN_VAL,		//值在两值之间
	ADD_UNKNOW_VAL,		//值增加了未知值
	ADD_ACCURATE_VAL,	//值增加了精确值
	SUB_UNKNOW_VAL,		//值减少了未知值
	SUB_ACCURATE_VAL,	//值减少了精确值
	CHANGED_VAL,		//变动了的数值
	UNCHANGED_VAL,		//未变动的数值
};

enum SCAN_VALUE_TYPE
{
	_1 = 0,			// 1字节
	_2,				// 2字节
	_4,				// 4字节
	_8,				// 8字节
	_FLOAT = _4,	// 单精度浮点数
	_DOUBLE = _8,	// 双精度浮点数
};

/*
内存搜索数值
hProcess：被搜索的进程句柄
vScanMemMapsJobList: 被搜索的进程内存区域
value1: 要搜索的数值1
value2: 要搜索的数值2
errorRange: 误差范围（当搜索的数值为float或者double时此值有效，其余情况请填写0）
scanType：搜索类型：
				ACCURATE_VAL精确数值（value1生效）、
				LARGER_THAN_VAL值大于（value1生效）、
				LESS_THAN_VAL值小于（value1生效）、
				BETWEEN_VAL值在两值之间（value1、value2生效）
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
*/
template<typename T> static void SearchMemoryThread(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess,
	SafeVector<MEM_SECTION_INFO> & vScanMemMapsJobList,
	T value1, T value2, float errorRange, SCAN_TYPE scanType, int nThreadCount,
	size_t nFastScanAlignment,
	SafeVector<ADDR_RESULT_INFO> & vResultList,
	SafeVector<MEM_SECTION_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal = nullptr);


/*
再次搜索内存数值
hProcess：被搜索的进程句柄
vScanMemAddrList: 需要再次搜索的内存地址列表
value1: 要搜索数值1
value2: 要搜索数值2
errorRange: 误差范围（当搜索的数值为float或者double时此值有效，其余情况请填写0）
scanType：搜索类型：
				ACCURATE_VAL精确数值（value1生效）、
				LARGER_THAN_VAL值大于（value1生效）、
				LESS_THAN_VAL值小于（value1生效）、
				BETWEEN_VAL值在两值之间（value1、value2生效）、
				ADD_UNKNOW_VAL值增加了未知值（value1、value2均无效），
				ADD_ACCURATE_VAL值增加了精确值（value1生效），
				SUB_UNKNOW_VAL值减少了未知值（value1、value2均无效），
				SUB_ACCURATE_VAL值减少了精确值（value1生效），
				CHANGED_VAL变动了的数值（value1、value2均无效），
				UNCHANGED_VAL未变动的数值（value1、value2均无效）
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
*/
template<typename T> static void SearchNextMemoryThread(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess,
	SafeVector<ADDR_RESULT_INFO> & vScanMemAddrList,
	T value1, T value2, float errorRange, SCAN_TYPE scanType, int nThreadCount,
	SafeVector<ADDR_RESULT_INFO> & vResultList,
	SafeVector<ADDR_RESULT_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal = nullptr);

/*
内存批量搜索值在两值之间的内存地址
hProcess：被搜索的进程句柄
vScanMemMapsJobList: 被搜索的进程内存区域
betweenValueList: 待搜索的两值之间的数值数组
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
*/
template<typename T> static void SearchMemoryBatchBetweenValThread(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess,
	SafeVector<MEM_SECTION_INFO> & vScanMemMapsJobList,
	const std::vector<BATCH_BETWEEN_VAL<T>> & betweenValueList, int nThreadCount,
	size_t nFastScanAlignment,
	SafeVector<BATCH_BETWEEN_VAL_ADDR_RESULT<T>> & vResultList,
	SafeVector<MEM_SECTION_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal = nullptr);


/*
搜索已拷贝的进程内存数值
vCopyProcessMemDataList: 需要再次搜索的已拷贝的进程内存列表
value1: 要搜索的数值1
value2: 要搜索的数值2
errorRange: 误差范围（当搜索的数值为float或者double时此值有效，其余情况请填写0）
scanType：搜索类型：
				ACCURATE_VAL精确数值（value1生效）、
				LARGER_THAN_VAL值大于（value1生效）、
				LESS_THAN_VAL值小于（value1生效）、
				BETWEEN_VAL值在两值之间（value1、value2生效）、
				ADD_UNKNOW_VAL值增加了未知值（value1、value2均无效），ADD_ACCURATE_VAL值增加了精确值（value1生效），SUB_UNKNOW_VAL值减少了未知值（value1、value2均无效），SUB_ACCURATE_VAL值减少了精确值（value1生效），CHANGED_VAL变动了的数值（value1、value2均无效），UNCHANGED_VAL未变动的数值（value1、value2均无效）
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
*/
template<typename T> static void SearchCopyProcessMemThread(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess,
	SafeVector<COPY_MEM_INFO> & vCopyProcessMemDataList,
	T value1, T value2, float errorRange, SCAN_TYPE scanType, int nThreadCount,
	size_t nFastScanAlignment,
	SafeVector<ADDR_RESULT_INFO> & vResultList,
	SafeVector<ADDR_RESULT_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal = nullptr);


/*
内存搜索（搜索文本特征码）
hProcess：被搜索的进程句柄
vScanMemMapsList: 被搜索的进程内存区域
strFeaturesByte: 十六进制的字节特征码，搜索规则与OD相同，如“68 00 00 00 40 ?3 7? ?? ?? ?? ?? ?? ?? 50 E8”
nThreadCount: 用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
std::atomic<bool> * pForceStopSignal: 强制中止所有任务信号
*/
static void SearchMemoryBytesThread(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess,
	SafeVector<MEM_SECTION_INFO> & vScanMemMapsList,
	std::string strFeaturesByte,
	int nThreadCount,
	size_t nFastScanAlignment,
	SafeVector<ADDR_RESULT_INFO> & vResultList,
	SafeVector<ADDR_RESULT_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal = nullptr);


/*
内存搜索（搜索内存特征码）
hProcess：被搜索的进程句柄
vScanMemMapsList: 被搜索的进程内存区域
vFeaturesByte：字节特征码，如“char vBytes[] = {'\x68', '\x00', '\x00', '\x00', '\x40', '\x?3', '\x??', '\x7?', '\x??', '\x??', '\x??', '\x??', '\x??', '\x50', '\xE8};”
nFeaturesByteLen：字节特征码长度
vFuzzyBytes：模糊字节，不变动的位置用1表示，变动的位置用0表示，如“char vFuzzy[] = {'\x11', '\x11', '\x11', '\x11', '\x11', '\x01', '\x00', '\x10', '\x00', '\x00', '\x00', '\x00', '\x00', '\x11', '\x11};”
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
*/
static void SearchMemoryBytesThread2(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess,
	SafeVector<MEM_SECTION_INFO> & vScanMemMapsList,
	const char vFeaturesByte[],
	size_t nFeaturesByteLen,
	char vFuzzyBytes[],
	int nThreadCount,
	size_t nFastScanAlignment,
	SafeVector<ADDR_RESULT_INFO> & vResultList,
	SafeVector<MEM_SECTION_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal = nullptr);

/*
再次搜索内存（搜索文本特征码）
hProcess：被搜索的进程句柄
vScanMemAddrList: 需要再次搜索的内存地址列表
strFeaturesByte: 十六进制的字节特征码，搜索规则与OD相同，如“68 00 00 00 40 ?3 7? ?? ?? ?? ?? ?? ?? 50 E8”
nThreadCount: 用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
*/
static void SearchNextMemoryBytesThread(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess,
	SafeVector<ADDR_RESULT_INFO> & vScanMemAddrList,
	std::string strFeaturesByte,
	int nThreadCount,
	size_t nFastScanAlignment,
	SafeVector<ADDR_RESULT_INFO> & vResultList,
	SafeVector<ADDR_RESULT_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal = nullptr);

/*
再次搜索内存（搜索内存特征码）
hProcess：被搜索的进程句柄
vScanMemAddrList: 需要再次搜索的内存地址列表
vFeaturesByte：字节特征码，如“char vBytes[] = {0x68, 0x00, 0x00, 0x00, 0x40, 0x?3, 0x??, 0x7?, 0x??, 0x??, 0x??, 0x??, 0x??, 0x50, 0xE8};”
nFeaturesByteLen：字节特征码长度
vFuzzyBytes：模糊字节，不变动的位置用1表示，变动的位置用0表示，如“char vFuzzy[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11};”
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
*/
static void SearchNextMemoryBytesThread2(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess,
	SafeVector<ADDR_RESULT_INFO> & vScanMemAddrList,
	char vFeaturesByte[],
	size_t nFeaturesByteLen,
	char vFuzzyBytes[],
	int nThreadCount,
	size_t nFastScanAlignment,
	SafeVector<ADDR_RESULT_INFO> & vResultList,
	SafeVector<ADDR_RESULT_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal = nullptr);


/*
拷贝进程内存数据
hProcess：被搜索的进程句柄
vScanMemMapsList: 被拷贝的进程内存区域
vOutputMemCopyList：拷贝进程内存数据的存放区域
vErrorMemCopyList：拷贝进程内存数据失败的存放区域
*/
static void CopyProcessMemDataThread(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess,
	SafeVector<MEM_SECTION_INFO> & vScanMemMapsList,
	int scanValueType,
	SafeVector<COPY_MEM_INFO> & vOutputMemCopyList,
	SafeVector<MEM_SECTION_INFO> & vErrorMemCopyList,
	std::atomic<bool> * pForceStopSignal = nullptr);

/*
多线程执行任务
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
void OnThreadExecute(size_t thread_id，std::atomic<bool> *pForceStopSignal)：每条线程要执行的任务回调（线程ID号，中止信号）
std::atomic<bool> * pForceStopSignal: 强制中止所有任务信号
*/
static void MultiThreadExecuteTask(
	int nThreadCount,
	std::function<void(size_t thread_id, std::atomic<bool> *pForceStopSignal)> OnThreadExecut,
	std::atomic<bool> * pForceStopSignal = nullptr);



/*
寻找精确数值
使用方法:
dwAddr：要搜索的缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索的数值
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
*/
template<typename T> static inline void FindValue(size_t dwAddr, size_t dwLen, T value, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr);

/*
寻找大于的数值
使用方法:
dwAddr：要搜索的缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索大于的数值
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
*/
template<typename T> static inline void FindMax(size_t dwAddr, size_t dwLen, T value, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr);

/*
寻找小于的数值
使用方法:
dwAddr：要搜索的缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索小于的char数值
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
*/
template<typename T> static inline void FindMin(size_t dwAddr, size_t dwLen, T value, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr);

/*
寻找两者之间的数值
使用方法:
dwAddr：要搜索的缓冲区地址
dwLen：要搜索的缓冲区大小
value1: 要搜索的数值1
value2: 要搜索的数值2
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
*/
template<typename T> static inline void FindBetween(size_t dwAddr, size_t dwLen, T value1, T value2, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr);

/*
寻找增加的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
*/
template<typename T> static inline void FindUnknowAdd(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr);

/*
寻找增加了已知值的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索增加的已知值
errorRange: 误差范围（当搜索的数值为float或者double时此值有效，其余情况请填写0）
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
*/
template<typename T> static inline void FindAdd(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, T value, float errorRange, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr);

/*
寻找减少的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
*/
template<typename T> static inline void FindUnknowSum(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr);

/*
寻找减少了已知值的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索减少的已知值
errorRange: 误差范围（当搜索的数值为float或者double时此值有效，其余情况请填写0）
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
*/
template<typename T> static inline void FindSum(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, T value, float errorRange, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr);

/*
寻找变动的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
*/
template<typename T> static inline void FindChanged(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr);

/*
寻找未变动的数值
使用方法:
dwOldAddr：要搜索的旧数据缓冲区地址
dwNewAddr：要搜索的新数据缓冲区地址
dwLen：要搜索的缓冲区大小
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
*/
template<typename T> static inline void FindNoChange(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr);



/*
寻找字节集
dwAddr: 要搜索的缓冲区地址
dwLen: 要搜索的缓冲区长度
bMask: 要搜索的字节集
szMask: 要搜索的字节集模糊码（xx=已确定字节，??=可变化字节，支持x?、?x）
nMaskLen: 要搜索的字节集长度
nFastScanAlignment: 为快速扫描的对齐位数，CE默认为1
vOutputAddr: 搜索出来的结果地址存放数组
使用方法：
std::vector<size_t> vAddr;
FindFeaturesBytes((size_t)lpBuffer, dwBufferSize, (PBYTE);"\x8B\xE8\x00\x00\x00\x00\x33\xC0\xC7\x06\x00\x00\x00\x00\x89\x86\x40", "xxxx??xx?xx?xxxxx",17,1,vAddr);;
*/
static inline void FindFeaturesBytes(size_t dwAddr, size_t dwLen, unsigned char *bMask, const char* szMask, size_t nMaskLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr);

/*
寻找字节集
dwWaitSearchAddress: 要搜索的缓冲区地址
dwLen: 要搜索的缓冲区长度
bForSearch: 要搜索的字节集
ifLen: 要搜索的字节集长度
nFastScanAlignment: 为快速扫描的对齐位数，CE默认为1
vOutputAddr: 搜索出来的结果地址存放数组
使用方法：
std::vector<size_t> vAddr;
FindFeaturesBytes((size_t)lpBuffer, dwBufferSize, (PBYTE)"\x8B\xE8\x00\x00\x00\x00\x33\xC0\xC7\x06\x00\x00\x00\x00\x89\x86\x40", 17,1,vAddr);
*/
static inline void FindBytes(size_t dwWaitSearchAddress, size_t dwLen, unsigned char *bForSearch, size_t ifLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr);

static inline std::string& replace_all_distinct(std::string& str, const std::string& old_value, const std::string& new_value);


/*
内存搜索数值
hProcess：被搜索的进程句柄
vScanMemMapsJobList: 被搜索的进程内存区域
value1: 要搜索的数值1
value2: 要搜索的数值2
errorRange: 误差范围（当搜索的数值为float或者double时此值有效，其余情况请填写0）
scanType：搜索类型：
				ACCURATE_VAL精确数值（value1生效）、
				LARGER_THAN_VAL值大于（value1生效）、
				LESS_THAN_VAL值小于（value1生效）、
				BETWEEN_VAL值在两值之间（value1、value2生效）
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
std::atomic<bool> * pForceStopSignal: 强制中止所有任务信号
*/
template<typename T> static void SearchMemoryThread(
	IMemReaderWriterProxy* IReadWriteProxy, 
	uint64_t hProcess, 
	SafeVector<MEM_SECTION_INFO> & vScanMemMapsJobList,
	T value1, T value2, float errorRange, SCAN_TYPE scanType, int nThreadCount,
	size_t nFastScanAlignment, 
	SafeVector<ADDR_RESULT_INFO> & vResultList,
	SafeVector<MEM_SECTION_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal/* = nullptr*/)
{
	//获取当前系统内存大小
#ifdef __linux__
	struct sysinfo si;
	sysinfo(&si);
	size_t nMaxMemSize = si.totalram;
#else
	MEMORYSTATUS memoryStatus;
	memset(&memoryStatus, 0, sizeof(MEMORYSTATUS));
	memoryStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&memoryStatus);
	size_t nMaxMemSize = memoryStatus.dwTotalPhys;
#endif

	MultiThreadExecuteTask(nThreadCount, [IReadWriteProxy, nMaxMemSize, hProcess,
		&vScanMemMapsJobList, &vErrorList, value1, value2, errorRange, scanType,
		&vResultList, nFastScanAlignment](size_t thread_id, std::atomic<bool> * pForceStopSignal)->void {

		std::vector<ADDR_RESULT_INFO> vThreadOutput; //存放当前线程的搜索结果

		bool isFloatVal = typeid(T) == typeid(static_cast<float>(0)) || typeid(T) == typeid(static_cast<double>(0));

		//一个一个job拿会拖慢速度
		std::vector<MEM_SECTION_INFO> vTempJobMemSecInfo;
		while (vTempJobMemSecInfo.size() || vScanMemMapsJobList.get_vals(GET_JOB_COUNT, vTempJobMemSecInfo) || vScanMemMapsJobList.get_vals(1, vTempJobMemSecInfo)) {
			MEM_SECTION_INFO memSecInfo = vTempJobMemSecInfo.back();
			vTempJobMemSecInfo.pop_back();
			//当前内存区域块大小超过了最大内存申请的限制，所以跳过
			if (memSecInfo.nSectionSize > nMaxMemSize)
			{
				vErrorList.push_back(memSecInfo);
				continue;
			}

			unsigned char *pnew = new (std::nothrow) unsigned char[memSecInfo.nSectionSize];
			if (!pnew)
			{
				vErrorList.push_back(memSecInfo);
				continue;
			}
			memset(pnew, 0, memSecInfo.nSectionSize);


			std::shared_ptr<unsigned char> spMemBuf(pnew, std::default_delete<unsigned char[]>());
			size_t dwRead = 0;
			if (!IReadWriteProxy->ReadProcessMemory(hProcess, memSecInfo.npSectionAddr, spMemBuf.get(), memSecInfo.nSectionSize, &dwRead))
			{
				vErrorList.push_back(memSecInfo);
				continue;
			}
			//寻找数值
			std::vector<size_t >vFindAddr;

			switch (scanType)
			{
			case SCAN_TYPE::ACCURATE_VAL:
				//精确数值
				if (isFloatVal) {
					FindBetween<T>((size_t)(spMemBuf.get()),
						dwRead, value1 - errorRange, value1 + errorRange, nFastScanAlignment, vFindAddr);
				}
				else {
					FindValue<T>((size_t)spMemBuf.get(), dwRead, value1, nFastScanAlignment, vFindAddr);
				}
				
				break;
			case SCAN_TYPE::LARGER_THAN_VAL:
				//值大于
				FindMax<T>((size_t)spMemBuf.get(), dwRead, value1, nFastScanAlignment, vFindAddr);
				break;
			case SCAN_TYPE::LESS_THAN_VAL:
				//值小于
				FindMin<T>((size_t)spMemBuf.get(), dwRead, value1, nFastScanAlignment, vFindAddr);
				break;
			case SCAN_TYPE::BETWEEN_VAL:
				//值在两者之间于
				FindBetween<T>((size_t)spMemBuf.get(), dwRead, value1 < value2 ? value1 : value2, value1 < value2 ? value2 : value1, nFastScanAlignment, vFindAddr);
				break;
			default:
				break;
			}
			for (size_t addr : vFindAddr)
			{
				ADDR_RESULT_INFO aInfo;
				aInfo.addr = (uint64_t)((uint64_t)memSecInfo.npSectionAddr + (uint64_t)addr - (uint64_t)spMemBuf.get());
				aInfo.size = sizeof(T);

				std::shared_ptr<unsigned char> sp(new unsigned char[aInfo.size], std::default_delete<unsigned char[]>());
				memcpy(sp.get(), (void*)addr, aInfo.size);

				aInfo.spSaveData = sp;
				vThreadOutput.push_back(aInfo);
			}

		}


		//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
		for (ADDR_RESULT_INFO & newAddr : vThreadOutput) {
			vResultList.push_back(newAddr);
		}

	}, pForceStopSignal);
	vResultList.sort([](const ADDR_RESULT_INFO & a, const ADDR_RESULT_INFO & b) -> bool { return a.addr < b.addr; });
	return;
}


/*
再次搜索内存数值
hProcess：被搜索的进程句柄
vScanMemAddrList: 需要再次搜索的内存地址列表
value1: 要搜索数值1
value2: 要搜索数值2
errorRange: 误差范围（当搜索的数值为float或者double时此值有效，其余情况请填写0）
scanType：搜索类型：
				ACCURATE_VAL精确数值（value1生效）、
				LARGER_THAN_VAL值大于（value1生效）、
				LESS_THAN_VAL值小于（value1生效）、
				BETWEEN_VAL值在两值之间（value1、value2生效）、
				ADD_UNKNOW_VAL值增加了未知值（value1、value2均无效），
				ADD_ACCURATE_VAL值增加了精确值（value1生效），
				SUB_UNKNOW_VAL值减少了未知值（value1、value2均无效），
				SUB_ACCURATE_VAL值减少了精确值（value1生效），
				CHANGED_VAL变动了的数值（value1、value2均无效），
				UNCHANGED_VAL未变动的数值（value1、value2均无效）
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
std::atomic<bool> * pForceStopSignal: 强制中止所有任务信号
*/
template<typename T> static void SearchNextMemoryThread(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess, 
	SafeVector<ADDR_RESULT_INFO> & vScanMemAddrList,
	T value1, T value2, float errorRange, SCAN_TYPE scanType, int nThreadCount,
	SafeVector<ADDR_RESULT_INFO> & vResultList,
	SafeVector<ADDR_RESULT_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal/* = nullptr*/)
{
	//内存搜索线程
	MultiThreadExecuteTask(nThreadCount,
		[IReadWriteProxy, hProcess, &vScanMemAddrList, &vErrorList,
		value1, value2, errorRange, scanType, &vResultList](size_t thread_id, std::atomic<bool> * pForceStopSignal)->void {

		std::vector<ADDR_RESULT_INFO> vThreadOutput; //存放当前线程的搜索结果

		bool isFloatVal = typeid(T) == typeid(static_cast<float>(0)) || typeid(T) == typeid(static_cast<double>(0));

		//一个一个job拿会拖慢速度
		std::vector<ADDR_RESULT_INFO> vTempJobMemAddrInfo;
		while (vTempJobMemAddrInfo.size() || vScanMemAddrList.get_vals(GET_JOB_COUNT, vTempJobMemAddrInfo) || vScanMemAddrList.get_vals(1, vTempJobMemAddrInfo))
		{
			ADDR_RESULT_INFO memAddrJob = vTempJobMemAddrInfo.back();
			vTempJobMemAddrInfo.pop_back();

			T temp = 0;
			size_t dwRead = 0;
			if (!IReadWriteProxy->ReadProcessMemory(hProcess, memAddrJob.addr, &temp, sizeof(temp), &dwRead))
			{
				vErrorList.push_back(memAddrJob);
				continue;
			}
				
			if (dwRead != sizeof(T))
			{
				vErrorList.push_back(memAddrJob);
				continue;
			}

			//寻找数值
			switch (scanType)
			{
			case SCAN_TYPE::ACCURATE_VAL:
				if (isFloatVal) {
					//当是float、double数值的情况
					if ((value1 - errorRange) > temp || temp > (value1 + errorRange)) { continue; }
				}
				else {
					//精确数值
					if (temp != value1) { continue; }
				}
				break;
			case SCAN_TYPE::LARGER_THAN_VAL:
				//值大于的结果保留
				if (temp < value1) { continue; }
				break;
			case SCAN_TYPE::LESS_THAN_VAL:
				//值小于的结果保留
				if (temp > value1) { continue; }
				break;
			case SCAN_TYPE::BETWEEN_VAL:
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
			case SCAN_TYPE::ADD_UNKNOW_VAL:
				//增加的数值的结果保留
				if (temp <= *(T*)(memAddrJob.spSaveData.get())) { continue; }
				break;
			case SCAN_TYPE::ADD_ACCURATE_VAL:
				if (isFloatVal) {
					//float、double数值增加了的结果保留
					T* pOldData = (T*)&(*memAddrJob.spSaveData);
					T add = (temp)-(*pOldData);

					if ((value1 - errorRange) > add || add > (value1 + errorRange)) { continue; }
				}
				else {
					//数值增加了的结果保留
					if ( ((temp)-(*(T*)(memAddrJob.spSaveData.get()))) != value1) { continue; }

				}
				break;
			case SCAN_TYPE::SUB_UNKNOW_VAL:
				//减少的数值的结果保留
				if (temp >= *(T*)(memAddrJob.spSaveData.get())) { continue; }
				break;
			case SCAN_TYPE::SUB_ACCURATE_VAL:
				if (isFloatVal) {
					//float、double数值减少了的结果保留
					T* pOldData = (T*)&(*memAddrJob.spSaveData);
					T sum = (*pOldData) - (temp);

					if ((value1 - errorRange) > sum || sum > (value1 + errorRange)) { continue; }

				}
				else {
					//数值减少了的结果保留
					if (((*(T*)(memAddrJob.spSaveData.get())) - (temp)) != value1) { continue; }
				}
				break;
			case SCAN_TYPE::CHANGED_VAL:
				if (isFloatVal) {
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
				else {
					//变动的数值的结果保留
					if (temp == *(T*)(memAddrJob.spSaveData.get())) { continue; }
				}
				break;
			case SCAN_TYPE::UNCHANGED_VAL:
				if (isFloatVal) {
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
				else {
					//未变动的数值的结果保留
					if (temp != *(T*)(memAddrJob.spSaveData.get())) { continue; }
				}

				break;
			default:
				break;
			}
			
			
			if (memAddrJob.size != sizeof(T))
			{
				memAddrJob.size = sizeof(T);
				std::shared_ptr<unsigned char> sp(new unsigned char[memAddrJob.size], std::default_delete<unsigned char[]>());
				memAddrJob.spSaveData = sp;
			}
			memcpy(&(*memAddrJob.spSaveData), &temp, 1);
			vThreadOutput.push_back(memAddrJob);
		}
		//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
		for (ADDR_RESULT_INFO & newAddr : vThreadOutput)
		{
			vResultList.push_back(newAddr);
		}

	}, pForceStopSignal);
	vResultList.sort([](const ADDR_RESULT_INFO & a, const ADDR_RESULT_INFO & b) -> bool { return a.addr < b.addr; });
	return;
}


/*
内存批量搜索值在两值之间的内存地址
hProcess：被搜索的进程句柄
vScanMemMapsJobList: 被搜索的进程内存区域
betweenValueList: 待搜索的两值之间的数值数组
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
std::atomic<bool> * pForceStopSignal: 强制中止所有任务信号
*/
template<typename T> static void SearchMemoryBatchBetweenValThread(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess,
	SafeVector<MEM_SECTION_INFO> & vScanMemMapsJobList,
	const std::vector<BATCH_BETWEEN_VAL<T>> & betweenValueList, int nThreadCount,
	size_t nFastScanAlignment,
	SafeVector<BATCH_BETWEEN_VAL_ADDR_RESULT<T>> & vResultList,
	SafeVector<MEM_SECTION_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal/* = nullptr*/)
{
	//获取当前系统内存大小
#ifdef __linux__
	struct sysinfo si;
	sysinfo(&si);
	size_t nMaxMemSize = si.totalram;
#else
	MEMORYSTATUS memoryStatus;
	memset(&memoryStatus, 0, sizeof(MEMORYSTATUS));
	memoryStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&memoryStatus);
	size_t nMaxMemSize = memoryStatus.dwTotalPhys;
#endif

	MultiThreadExecuteTask(nThreadCount,
		[IReadWriteProxy, nMaxMemSize, hProcess, &vScanMemMapsJobList,
		&vErrorList, &betweenValueList, &vResultList, nFastScanAlignment](size_t thread_id, std::atomic<bool> * pForceStopSignal)->void {

		std::vector<BATCH_BETWEEN_VAL_ADDR_RESULT<T>> vThreadOutput; //存放当前线程的搜索结果

		MEM_SECTION_INFO memSecInfo;
		while (vScanMemMapsJobList.get_val(memSecInfo)) //这里一个一个取更适合
		{
			//当前内存区域块大小超过了最大内存申请的限制，所以跳过
			if (memSecInfo.nSectionSize > nMaxMemSize)
			{
				vErrorList.push_back(memSecInfo);
				continue;
			}

			unsigned char *pnew = new (std::nothrow) unsigned char[memSecInfo.nSectionSize];
			if (!pnew)
			{
				vErrorList.push_back(memSecInfo);
				continue;
			}
			memset(pnew, 0, memSecInfo.nSectionSize);


			std::shared_ptr<unsigned char> spMemBuf(pnew, std::default_delete<unsigned char[]>());
			size_t dwRead = 0;
			if (!IReadWriteProxy->ReadProcessMemory(hProcess, memSecInfo.npSectionAddr, spMemBuf.get(), memSecInfo.nSectionSize, &dwRead))
			{
				vErrorList.push_back(memSecInfo);
				continue;
			}
			for (size_t i = 0; i < betweenValueList.size(); i++) {
				auto & item = betweenValueList.at(i);

				//寻找数值
				std::vector<size_t >vFindAddr;

				//值在两者之间于
				FindBetween<T>((size_t)spMemBuf.get(), dwRead, item.val1 < item.val2 ? item.val1 : item.val2, item.val1 < item.val2 ? item.val2 : item.val1, nFastScanAlignment, vFindAddr);

				for (size_t addr : vFindAddr) {
					BATCH_BETWEEN_VAL_ADDR_RESULT<T> baInfo;
					baInfo.addrInfo.addr = (uint64_t)((uint64_t)memSecInfo.npSectionAddr + (uint64_t)addr - (uint64_t)spMemBuf.get());
					baInfo.addrInfo.size = sizeof(T);
					std::shared_ptr<unsigned char> sp(new unsigned char[baInfo.addrInfo.size], std::default_delete<unsigned char[]>());
					memcpy(sp.get(), (void*)addr, baInfo.addrInfo.size);
					baInfo.addrInfo.spSaveData = sp;

					//原始条件
					baInfo.originalCondition = item;

					vThreadOutput.push_back(baInfo);
				}
			}
		}

		//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
		for (BATCH_BETWEEN_VAL_ADDR_RESULT<T> & newAddr : vThreadOutput) {
			vResultList.push_back(newAddr);
		}
	}, pForceStopSignal);

	vResultList.sort([](const BATCH_BETWEEN_VAL_ADDR_RESULT<T> & a, const BATCH_BETWEEN_VAL_ADDR_RESULT<T> & b) -> bool { return a.addrInfo.addr < b.addrInfo.addr; });
	return;
}
/*
搜索已拷贝的进程内存数值
vCopyProcessMemDataList: 需要再次搜索的已拷贝的进程内存列表
value1: 要搜索的数值1
value2: 要搜索的数值2
errorRange: 误差范围（当搜索的数值为float或者double时此值有效，其余情况请填写0）
scanType：搜索类型：
				ACCURATE_VAL精确数值（value1生效）、
				LARGER_THAN_VAL值大于（value1生效）、
				LESS_THAN_VAL值小于（value1生效）、
				BETWEEN_VAL值在两值之间（value1、value2生效）、
				ADD_UNKNOW_VAL值增加了未知值（value1、value2均无效），ADD_ACCURATE_VAL值增加了精确值（value1生效），SUB_UNKNOW_VAL值减少了未知值（value1、value2均无效），SUB_ACCURATE_VAL值减少了精确值（value1生效），CHANGED_VAL变动了的数值（value1、value2均无效），UNCHANGED_VAL未变动的数值（value1、value2均无效）
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
std::atomic<bool> * pForceStopSignal: 强制中止所有任务信号
*/
template<typename T> static void SearchCopyProcessMemThread(
	IMemReaderWriterProxy* IReadWriteProxy, 
	uint64_t hProcess, 
	SafeVector<COPY_MEM_INFO> & vCopyProcessMemDataList,
	T value1, T value2, float errorRange, SCAN_TYPE scanType, int nThreadCount,
	size_t nFastScanAlignment, 
	SafeVector<ADDR_RESULT_INFO> & vResultList,
	SafeVector<COPY_MEM_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal/* = nullptr*/)
{

	MultiThreadExecuteTask(nThreadCount,
		[IReadWriteProxy, &vCopyProcessMemDataList, hProcess, value1, value2, errorRange,
		scanType, nFastScanAlignment, &vResultList, &vErrorList](size_t thread_id, std::atomic<bool> * pForceStopSignal)->void {

		std::vector<ADDR_RESULT_INFO> vThreadOutput; //存放当前线程的搜索结果


		bool isFloatVal = typeid(T) == typeid(static_cast<float>(0)) || typeid(T) == typeid(static_cast<double>(0));

		//一个一个job拿会拖慢速度
		std::vector<COPY_MEM_INFO> vTempJobMemSecInfo;
		while (vTempJobMemSecInfo.size() || vCopyProcessMemDataList.get_vals(GET_JOB_COUNT, vTempJobMemSecInfo) || vCopyProcessMemDataList.get_vals(1, vTempJobMemSecInfo))
		{
			COPY_MEM_INFO memSecInfo = vTempJobMemSecInfo.back();
			vTempJobMemSecInfo.pop_back();

			//寻找数值
			std::vector<size_t >vFindAddr;
			std::shared_ptr<unsigned char> spMemBuf = nullptr;
			size_t dwRead = 0;
			switch (scanType)
			{
			case SCAN_TYPE::ACCURATE_VAL:
				//精确数值
				if (isFloatVal) {
					FindBetween<T>((size_t)(memSecInfo.spSaveMemBuf.get()),
						memSecInfo.nSectionSize, value1 - errorRange, value1 + errorRange, nFastScanAlignment, vFindAddr);
				}
				else {
					FindValue<T>((size_t)(memSecInfo.spSaveMemBuf.get()),
						memSecInfo.nSectionSize, value1, nFastScanAlignment, vFindAddr);
				}
				
				break;
			case SCAN_TYPE::LARGER_THAN_VAL:
				//值大于
				FindMax<T>((size_t)(memSecInfo.spSaveMemBuf.get()),
					memSecInfo.nSectionSize, value1, nFastScanAlignment, vFindAddr);
				break;
			case SCAN_TYPE::LESS_THAN_VAL:
				//值小于
				FindMin<T>((size_t)(memSecInfo.spSaveMemBuf.get()),
					memSecInfo.nSectionSize, value1, nFastScanAlignment, vFindAddr);
				break;
			case SCAN_TYPE::BETWEEN_VAL:
				//值在两者之间于
				FindBetween<T>((size_t)(memSecInfo.spSaveMemBuf.get()),
					memSecInfo.nSectionSize, value1 < value2 ? value1 : value2, value1 < value2 ? value2 : value1, nFastScanAlignment, vFindAddr);
				break;
			case SCAN_TYPE::ADD_UNKNOW_VAL:
			case SCAN_TYPE::ADD_ACCURATE_VAL:
			case SCAN_TYPE::SUB_UNKNOW_VAL:
			case SCAN_TYPE::SUB_ACCURATE_VAL:
			case SCAN_TYPE::CHANGED_VAL:
			case SCAN_TYPE::UNCHANGED_VAL:{
				unsigned char *pnew = new (std::nothrow) unsigned char[memSecInfo.nSectionSize];
				if (!pnew)
				{
					vErrorList.push_back(memSecInfo);
					/*cout << "malloc "<< memSecInfo.nSectionSize  << " failed."<< endl;*/ 
					continue;
				}
				memset(pnew, 0, memSecInfo.nSectionSize);

				std::shared_ptr<unsigned char> sp(pnew, std::default_delete<unsigned char[]>());

				if (!IReadWriteProxy->ReadProcessMemory(hProcess, memSecInfo.npSectionAddr, sp.get(), memSecInfo.nSectionSize, &dwRead))
				{
					vErrorList.push_back(memSecInfo);
					continue;
				}
				spMemBuf = sp;
				break;}
			default:
				break;
			}

			switch (scanType)
			{
			case SCAN_TYPE::ADD_UNKNOW_VAL:
				//值增加了未知值
				FindUnknowAdd<T>((size_t)(memSecInfo.spSaveMemBuf.get()), (size_t)spMemBuf.get(), dwRead, nFastScanAlignment, vFindAddr);
				break;
			case SCAN_TYPE::ADD_ACCURATE_VAL:
				//值增加了精确值
				FindAdd<T>((size_t)memSecInfo.spSaveMemBuf.get(), (size_t)spMemBuf.get(), value1, errorRange, dwRead, nFastScanAlignment, vFindAddr);
				break;
			case SCAN_TYPE::SUB_UNKNOW_VAL:
				//值减少了未知值
				FindUnknowSum<T>((size_t)memSecInfo.spSaveMemBuf.get(), (size_t)spMemBuf.get(), dwRead, nFastScanAlignment, vFindAddr);
				break;
			case SCAN_TYPE::SUB_ACCURATE_VAL:
				//值减少了精确值
				FindSum<T>((size_t)memSecInfo.spSaveMemBuf.get(), (size_t)spMemBuf.get(), value1, errorRange, dwRead, nFastScanAlignment, vFindAddr);
				break;
			case SCAN_TYPE::CHANGED_VAL:
				//变动的数值
				FindChanged<T>((size_t)memSecInfo.spSaveMemBuf.get(), (size_t)spMemBuf.get(), dwRead, nFastScanAlignment, vFindAddr);
				break;
			case SCAN_TYPE::UNCHANGED_VAL:
				//未变动的数值
				FindNoChange<T>((size_t)memSecInfo.spSaveMemBuf.get(), (size_t)spMemBuf.get(), dwRead, nFastScanAlignment, vFindAddr);
				break;
			default:
				break;
			}
		
			for (size_t addr : vFindAddr)
			{
				ADDR_RESULT_INFO aInfo;
				aInfo.addr = (uint64_t)((uint64_t)memSecInfo.npSectionAddr + (uint64_t)addr - (uint64_t)memSecInfo.spSaveMemBuf.get());
				aInfo.size = sizeof(T);
				std::shared_ptr<unsigned char> sp(new unsigned char[aInfo.size], std::default_delete<unsigned char[]>());
				memcpy(sp.get(), (void*)addr, aInfo.size);
				aInfo.spSaveData = sp;
				vThreadOutput.push_back(aInfo);
			}
		}
		//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
		for (ADDR_RESULT_INFO & newAddr : vThreadOutput)
		{
			vResultList.push_back(newAddr);
		}
	}, pForceStopSignal);
	vResultList.sort([](ADDR_RESULT_INFO & a, ADDR_RESULT_INFO & b) -> bool { return a.addr < b.addr; });
	return;

}



/*
内存搜索（搜索文本特征码）
hProcess：被搜索的进程句柄
vScanMemMapsList: 被搜索的进程内存区域
strFeaturesByte: 十六进制的字节特征码，搜索规则与OD相同，如“68 00 00 00 40 ?3 7? ?? ?? ?? ?? ?? ?? 50 E8”
nThreadCount: 用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
std::atomic<bool> * pForceStopSignal: 强制中止所有任务信号
*/
static void SearchMemoryBytesThread(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess,
	SafeVector<MEM_SECTION_INFO> & vScanMemMapsList,
	std::string strFeaturesByte,
	int nThreadCount,
	size_t nFastScanAlignment,
	SafeVector<ADDR_RESULT_INFO> & vResultList,
	SafeVector<MEM_SECTION_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal/* = nullptr*/)
{


	//预处理特征码
	replace_all_distinct(strFeaturesByte, " ", "");//去除空格
	if (strFeaturesByte.length() % 2)
	{
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
		else if (strByte.substr(0, 1) == "?")
		{
			upFuzzyBytesBuf[nFeaturesBytePos] = '\x01';
		}
		else if (strByte.substr(1, 2) == "?")
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
		IReadWriteProxy,
		hProcess,
		vScanMemMapsList,
		upFeaturesBytesBuf.get(),
		strFeaturesByte.length() / 2,
		upFuzzyBytesBuf.get(),
		nThreadCount,
		nFastScanAlignment,
		vResultList,
		vErrorList,
		pForceStopSignal);
}

/*
内存搜索（搜索内存特征码）
hProcess：被搜索的进程句柄
vScanMemMapsList: 被搜索的进程内存区域
vFeaturesByte：字节特征码，如“char vBytes[] = {'\x68', '\x00', '\x00', '\x00', '\x40', '\x?3', '\x??', '\x7?', '\x??', '\x??', '\x??', '\x??', '\x??', '\x50', '\xE8};”
nFeaturesByteLen：字节特征码长度
vFuzzyBytes：模糊字节，不变动的位置用1表示，变动的位置用0表示，如“char vFuzzy[] = {'\x11', '\x11', '\x11', '\x11', '\x11', '\x01', '\x00', '\x10', '\x00', '\x00', '\x00', '\x00', '\x00', '\x11', '\x11};”
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
std::atomic<bool> * pForceStopSignal: 强制中止所有任务信号
*/
static void SearchMemoryBytesThread2(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess,
	SafeVector<MEM_SECTION_INFO> & vScanMemMapsList,
	const char vFeaturesByte[],
	size_t nFeaturesByteLen,
	char vFuzzyBytes[],
	int nThreadCount,
	size_t nFastScanAlignment,
	SafeVector<ADDR_RESULT_INFO> & vResultList,
	SafeVector<MEM_SECTION_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal/* = nullptr*/)
{
	//获取当前系统内存大小
#ifdef __linux__
	struct sysinfo si;
	sysinfo(&si);
	size_t nMaxMemSize = si.totalram;
#else
	MEMORYSTATUS memoryStatus;
	memset(&memoryStatus, 0, sizeof(MEMORYSTATUS));
	memoryStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&memoryStatus);
	size_t nMaxMemSize = memoryStatus.dwTotalPhys;
#endif


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


	//内存搜索线程
	MultiThreadExecuteTask(nThreadCount,
		[IReadWriteProxy, nMaxMemSize, hProcess, &vScanMemMapsList, vFeaturesByte, strFuzzyCode, nFastScanAlignment, &vResultList, &vErrorList](size_t thread_id, std::atomic<bool> * pForceStopSignal)->void
	{
		std::string spStrFuzzyCode = strFuzzyCode;

		//是否启用特征码容错搜索
		int isSimpleSearch = (spStrFuzzyCode.find("?") == -1) ? 1 : 0;

		std::vector<ADDR_RESULT_INFO> vThreadOutput; //存放当前线程的搜索结果


		//一个一个job拿会拖慢速度
		std::vector<MEM_SECTION_INFO> vTempJobMemSecInfo;
		while (vTempJobMemSecInfo.size() || vScanMemMapsList.get_vals(GET_JOB_COUNT, vTempJobMemSecInfo) || vScanMemMapsList.get_vals(1, vTempJobMemSecInfo))
		{
			MEM_SECTION_INFO memSecInfo = vTempJobMemSecInfo.back();
			vTempJobMemSecInfo.pop_back();

			//当前内存区域块大小超过了最大内存申请的限制，所以跳过
			if (memSecInfo.nSectionSize > nMaxMemSize)
			{
				vErrorList.push_back(memSecInfo);
				continue;
			}
			unsigned char *pnew = new (std::nothrow) unsigned char[memSecInfo.nSectionSize];
			if (!pnew) { /*cout << "malloc "<< memSecInfo.nSectionSize  << " failed."<< endl;*/ continue; }
			memset(pnew, 0, memSecInfo.nSectionSize);

			std::shared_ptr<unsigned char> spMemBuf(pnew, std::default_delete<unsigned char[]>());
			size_t dwRead = 0;
			if (!IReadWriteProxy->ReadProcessMemory(hProcess, memSecInfo.npSectionAddr, spMemBuf.get(), memSecInfo.nSectionSize, &dwRead))
			{
				vErrorList.push_back(memSecInfo);
				continue;
			}

			//寻找字节集
			std::vector<size_t>vFindAddr;
			if (isSimpleSearch)
			{
				//不需要容错搜索
				FindBytes((size_t)spMemBuf.get(), dwRead, (unsigned char*)&vFeaturesByte[0], spStrFuzzyCode.length() / 2, nFastScanAlignment, vFindAddr);
			}
			else
			{
				//需要容错搜索
				FindFeaturesBytes((size_t)spMemBuf.get(), dwRead, (unsigned char*)&vFeaturesByte[0], spStrFuzzyCode.c_str(), spStrFuzzyCode.length() / 2, nFastScanAlignment, vFindAddr);
			}

			for (size_t addr : vFindAddr)
			{
				//保存搜索结果
				ADDR_RESULT_INFO aInfo;
				aInfo.addr = (uint64_t)((uint64_t)memSecInfo.npSectionAddr + (uint64_t)addr - (uint64_t)spMemBuf.get());
				aInfo.size = spStrFuzzyCode.length() / 2;
				std::shared_ptr<unsigned char> sp(new unsigned char[aInfo.size], std::default_delete<unsigned char[]>());
				memcpy(sp.get(), (void*)addr, aInfo.size);
				aInfo.spSaveData = sp;
				vThreadOutput.push_back(aInfo);
			}
		}

		//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
		for (ADDR_RESULT_INFO & newAddr : vThreadOutput)
		{
			vResultList.push_back(newAddr);
		}
		return;
	}, pForceStopSignal);
	vResultList.sort([](const ADDR_RESULT_INFO & a, const ADDR_RESULT_INFO & b) -> bool { return a.addr < b.addr; });
	return;
}


/*
再次搜索内存（搜索文本特征码）
hProcess：被搜索的进程句柄
vScanMemAddrList: 需要再次搜索的内存地址列表
strFeaturesByte: 十六进制的字节特征码，搜索规则与OD相同，如“68 00 00 00 40 ?3 7? ?? ?? ?? ?? ?? ?? 50 E8”
nThreadCount: 用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
*/
static void SearchNextMemoryBytesThread(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess,
	SafeVector<ADDR_RESULT_INFO> & vScanMemAddrList,
	std::string strFeaturesByte,
	int nThreadCount,
	size_t nFastScanAlignment,
	SafeVector<ADDR_RESULT_INFO> & vResultList,
	SafeVector<ADDR_RESULT_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal)
{
	//预处理特征码
	replace_all_distinct(strFeaturesByte, " ", "");//去除空格
	if (strFeaturesByte.length() % 2)
	{
		//设置搜索进度为100
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
		IReadWriteProxy,
		hProcess,
		vScanMemAddrList,
		upFeaturesBytesBuf.get(),
		strFeaturesByte.length() / 2,
		upFuzzyBytesBuf.get(),
		nThreadCount,
		nFastScanAlignment,
		vResultList,
		vErrorList,
		pForceStopSignal);

}

/*
再次搜索内存（搜索内存特征码）
hProcess：被搜索的进程句柄
vScanMemAddrList: 需要再次搜索的内存地址列表
vFeaturesByte：字节特征码，如“char vBytes[] = {0x68, 0x00, 0x00, 0x00, 0x40, 0x?3, 0x??, 0x7?, 0x??, 0x??, 0x??, 0x??, 0x??, 0x50, 0xE8};”
nFeaturesByteLen：字节特征码长度
vFuzzyBytes：模糊字节，不变动的位置用1表示，变动的位置用0表示，如“char vFuzzy[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11};”
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vResultList：存放实时搜索完成的结果地址
vErrorList：存放实时搜索失败的结果地址
std::atomic<bool> * pForceStopSignal: 强制中止所有任务信号
*/
static void SearchNextMemoryBytesThread2(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess,
	SafeVector<ADDR_RESULT_INFO> & vScanMemAddrList,
	char vFeaturesByte[],
	size_t nFeaturesByteLen,
	char vFuzzyBytes[],
	int nThreadCount,
	size_t nFastScanAlignment,
	SafeVector<ADDR_RESULT_INFO> & vResultList,
	SafeVector<ADDR_RESULT_INFO> & vErrorList,
	std::atomic<bool> * pForceStopSignal/* = nullptr*/)
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


	//内存搜索线程
	MultiThreadExecuteTask(nThreadCount,
		[IReadWriteProxy, &vScanMemAddrList, hProcess, vFeaturesByte, strFuzzyCode, nFastScanAlignment, &vResultList, &vErrorList](size_t thread_id, std::atomic<bool> * pForceStopSignal)->void
	{
		std::string spStrFuzzyCode = strFuzzyCode;
		//是否启用特征码容错搜索
		int isSimpleSearch = (spStrFuzzyCode.find("?") == -1) ? 1 : 0;


		std::vector<ADDR_RESULT_INFO> vThreadOutput; //存放当前线程的搜索结果

		//一个一个job拿会拖慢速度
		std::vector<ADDR_RESULT_INFO> vTempJobMemAddrInfo;
		while (vTempJobMemAddrInfo.size() || vScanMemAddrList.get_vals(GET_JOB_COUNT, vTempJobMemAddrInfo) || vScanMemAddrList.get_vals(1, vTempJobMemAddrInfo))
		{
			ADDR_RESULT_INFO memAddrJob = vTempJobMemAddrInfo.back();
			vTempJobMemAddrInfo.pop_back();

			size_t memBufSize = spStrFuzzyCode.length() / 2;
			unsigned char *pnew = new (std::nothrow) unsigned char[memBufSize];
			if (!pnew)
			{ 
				/*cout << "malloc "<< memSecInfo.nSectionSize  << " failed."<< endl;*/ 
				vErrorList.push_back(memAddrJob);
				continue;
			}
			memset(pnew, 0, memBufSize);

			std::shared_ptr<unsigned char> spMemBuf(pnew, std::default_delete<unsigned char[]>());
			size_t dwRead = 0;
			if (!IReadWriteProxy->ReadProcessMemory(hProcess, memAddrJob.addr, spMemBuf.get(), memBufSize, &dwRead))
			{
				vErrorList.push_back(memAddrJob);
				continue;
			}
			if (dwRead != memBufSize)
			{
				vErrorList.push_back(memAddrJob);
				continue;
			}
			//寻找字节集
			std::vector<size_t>vFindAddr;


			if (isSimpleSearch)
			{
				//不需要容错搜索
				FindBytes((size_t)spMemBuf.get(), dwRead, (unsigned char*)&vFeaturesByte[0], spStrFuzzyCode.length() / 2, nFastScanAlignment, vFindAddr);
			}
			else
			{
				//需要容错搜索
				FindFeaturesBytes((size_t)spMemBuf.get(), dwRead, (unsigned char*)&vFeaturesByte[0], spStrFuzzyCode.c_str(), spStrFuzzyCode.length() / 2, nFastScanAlignment, vFindAddr);
			}

			if (vFindAddr.size())
			{
				//保存搜索结果
				memcpy(memAddrJob.spSaveData.get(), spMemBuf.get(), memBufSize);
				vThreadOutput.push_back(memAddrJob);
			}

		}
		//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
		for (ADDR_RESULT_INFO & newAddr : vThreadOutput)
		{
			vResultList.push_back(newAddr);
		}

	}, pForceStopSignal);
		
	vResultList.sort([](const ADDR_RESULT_INFO & a, const ADDR_RESULT_INFO & b) -> bool { return a.addr < b.addr; });
	return;
}

/*
拷贝进程内存数据
hProcess：被搜索的进程句柄
vScanMemMapsList: 被拷贝的进程内存区域
nThreadCount：用于拷贝内存的线程数，推荐设置为CPU数量
vOutputMemCopyList：拷贝进程内存数据的存放区域
vErrorMemCopyList：拷贝进程内存数据失败的存放区域
std::atomic<bool> * pForceStopSignal: 强制中止所有任务信号
*/
static void CopyProcessMemDataThread(
	IMemReaderWriterProxy* IReadWriteProxy,
	uint64_t hProcess,
	SafeVector<MEM_SECTION_INFO> & vScanMemMapsList,
	int nThreadCount,
	SafeVector<COPY_MEM_INFO> & vOutputMemCopyList,
	SafeVector<MEM_SECTION_INFO> & vErrorMemCopyList,
	std::atomic<bool> * pForceStopSignal/* = nullptr*/)
{

	//获取当前系统内存大小
#ifdef __linux__
	struct sysinfo si;
	sysinfo(&si);
	size_t nMaxMemSize = si.totalram;
#else
	MEMORYSTATUS memoryStatus;
	memset(&memoryStatus, 0, sizeof(MEMORYSTATUS));
	memoryStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&memoryStatus);
	size_t nMaxMemSize = memoryStatus.dwTotalPhys;
#endif

	MultiThreadExecuteTask(nThreadCount,
		[IReadWriteProxy, nMaxMemSize, hProcess, &vScanMemMapsList, &vErrorMemCopyList, &vOutputMemCopyList](size_t thread_id, std::atomic<bool> * pForceStopSignal)->void
	{
		std::vector<COPY_MEM_INFO> vThreadOutput; //存放当前线程的搜索结果

		//一个一个job拿会拖慢速度
		std::vector<MEM_SECTION_INFO> vTempJobMemSecInfo;
		while (vTempJobMemSecInfo.size() || vScanMemMapsList.get_vals(GET_JOB_COUNT, vTempJobMemSecInfo) || vScanMemMapsList.get_vals(1, vTempJobMemSecInfo)) {
			MEM_SECTION_INFO memSecInfo = vTempJobMemSecInfo.back();
			vTempJobMemSecInfo.pop_back();
			//当前内存区域块大小超过了最大内存申请的限制，所以跳过
			if (memSecInfo.nSectionSize > nMaxMemSize)
			{
				vErrorMemCopyList.push_back(memSecInfo);
				continue;
			}

			unsigned char *pnew = new (std::nothrow) unsigned char[memSecInfo.nSectionSize];
			if (!pnew)
			{
				vErrorMemCopyList.push_back(memSecInfo);
				continue;
			}
			memset(pnew, 0, memSecInfo.nSectionSize);

			std::shared_ptr<unsigned char>spMemBuf(pnew, std::default_delete<unsigned char[]>());
			size_t dwRead = 0;
			if (!IReadWriteProxy->ReadProcessMemory(hProcess, memSecInfo.npSectionAddr, spMemBuf.get(), memSecInfo.nSectionSize, &dwRead)) {
				vErrorMemCopyList.push_back(memSecInfo);
				continue;
			}
			COPY_MEM_INFO copyMem;
			copyMem.npSectionAddr = memSecInfo.npSectionAddr;
			copyMem.nSectionSize = dwRead;
			copyMem.spSaveMemBuf = spMemBuf;
			vThreadOutput.push_back(copyMem);
			if (pForceStopSignal && *pForceStopSignal) {
				break;
			}
		}
		//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
		for (auto & item : vThreadOutput) {
			vOutputMemCopyList.push_back(item);
		}
	}, pForceStopSignal);

	//vOutputMemCopyList.sort([](const COPY_MEM_INFO & a, const COPY_MEM_INFO & b) -> bool { return a.npSectionAddr < b.npSectionAddr; });
}


	
/*
多线程执行任务
nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
void OnThreadExecute(size_t thread_id，std::atomic<bool> *pForceStopSignal)：每条线程要执行的任务回调（线程ID号，中止信号）
std::atomic<bool> * pForceStopSignal: 强制中止所有任务信号
*/
static void MultiThreadExecuteTask(
	int nThreadCount,
	std::function<void(size_t thread_id, std::atomic<bool> *pForceStopSignal)> OnThreadExecute,
	std::atomic<bool> * pForceStopSignal/* = nullptr*/)
{

	std::vector<std::shared_ptr<std::mutex>> vspMtxThreadExist;
	std::vector<std::shared_ptr<std::atomic<bool>>> vbThreadStarted;
	for (int i = 0; i < nThreadCount; ++i) {
		vspMtxThreadExist.push_back(std::make_shared<std::mutex>());
		vbThreadStarted.push_back(std::make_shared<std::atomic<bool>>(false));
	}


	//开始分配线程搜索内存
	for (int i = 0; i < nThreadCount; i++) {
		std::shared_ptr<std::mutex> spMtxThread = vspMtxThreadExist[i];
		std::shared_ptr<std::atomic<bool>> spnThreadStarted = vbThreadStarted[i];
		//工作线程
		std::thread td(
			[i, OnThreadExecute, spMtxThread, spnThreadStarted, pForceStopSignal]()->void {
			std::lock_guard<std::mutex> mlock(*spMtxThread);
			spnThreadStarted->store(true);
			OnThreadExecute(i, pForceStopSignal);
		});
		td.detach();
	}
	//等待所有线程结束汇总
	for (auto spnThreadStarted : vbThreadStarted) {
		while (!spnThreadStarted->load()) {
#ifdef __linux__
			sleep(0);
#else
			Sleep(0);
#endif
		}
	}
	for (auto spMtxThread : vspMtxThreadExist) {
		std::lock_guard<std::mutex> mlock(*spMtxThread);
	}
}





/*
寻找精确数值
使用方法:
dwAddr：要搜索的缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索的数值
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
*/
template<typename T> static inline void FindValue(size_t dwAddr, size_t dwLen, T value, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr)
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
		}
		continue;
	}
}

/*
寻找大于的数值
使用方法:
dwAddr：要搜索的缓冲区地址
dwLen：要搜索的缓冲区大小
value: 要搜索大于的数值
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
*/
template<typename T> static inline void FindMax(size_t dwAddr, size_t dwLen, T value, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr)
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
*/
template<typename T> static inline void FindMin(size_t dwAddr, size_t dwLen, T value, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr)
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
*/
template<typename T> static inline void FindBetween(size_t dwAddr, size_t dwLen, T value1, T value2, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr)
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

		if (cData >= value1 && cData <= value2)
		{
			vOutputAddr.push_back((size_t)((size_t)dwAddr + (size_t)i));
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
*/
template<typename T> static inline void FindUnknowAdd(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr)
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
errorRange: 误差范围（当搜索的数值为float或者double时此值有效，其余情况请填写0）
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
*/
template<typename T> static inline void FindAdd(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, T value, float errorRange, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr)
{
	vOutputAddr.clear();

	float value1, value2;
	if (errorRange)
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

		if (errorRange)
		{
			if (myAdd >= value1 && myAdd <= value2)
			{
				vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));

			}
		}
		else
		{
			if (myAdd == value)
			{
				vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));

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
*/
template<typename T> static inline void FindUnknowSum(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr)
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
errorRange: 误差范围（当搜索的数值为float或者double时此值有效，其余情况请填写0）
nFastScanAlignment为快速扫描的对齐位数，CE默认为1
vOutputAddr：存放搜索完成的结果地址
*/
template<typename T> static inline void FindSum(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, T value, float errorRange, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr)
{
	vOutputAddr.clear();

	float value1, value2;
	if (errorRange)
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

		if (errorRange)
		{
			if (mySum >= value1 && mySum <= value2)
			{
				vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));
			}
		}
		else
		{
			if (mySum == value)
			{
				vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));
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
*/
template<typename T> static inline void FindChanged(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr)
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
*/
template<typename T> static inline void FindNoChange(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr)
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
使用方法：
std::vector<size_t> vAddr;
FindFeaturesBytes((size_t)lpBuffer, dwBufferSize, (PBYTE)"\x8B\xE8\x00\x00\x00\x00\x33\xC0\xC7\x06\x00\x00\x00\x00\x89\x86\x40", "xxxx??xx?xx?xxxxx",17,1,vAddr);
*/
static inline void FindFeaturesBytes(size_t dwAddr, size_t dwLen, unsigned char *bMask, const char* szMask, size_t nMaskLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr)
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
使用方法：
std::vector<size_t> vAddr;
FindFeaturesBytes((size_t)lpBuffer, dwBufferSize, (PBYTE)"\x8B\xE8\x00\x00\x00\x00\x33\xC0\xC7\x06\x00\x00\x00\x00\x89\x86\x40", 17,1,vAddr);
*/
static inline void FindBytes(size_t dwWaitSearchAddress, size_t dwLen, unsigned char *bForSearch, size_t ifLen, size_t nFastScanAlignment, std::vector<size_t> & vOutputAddr)
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

