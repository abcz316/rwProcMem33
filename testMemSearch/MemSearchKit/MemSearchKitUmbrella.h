#ifndef MEM_SEARCH_KIT_UMBRELLA_H_
#define MEM_SEARCH_KIT_UMBRELLA_H_
#include "MemSearchKitCore.h"
#include "MemSearchKitReverseAddrOffsetLink.h"

namespace MemorySearchKit {
	/*
	内存搜索数值
	pReadWriteProxy：读取进程内存数据的接口
	hProcess：被搜索的进程句柄
	spvWaitScanMemSecList: 被搜索的进程内存区域
	value1: 要搜索的数值1
	value2: 要搜索的数值2
	errorRange: 误差范围（当搜索的数值为float或者double时此值有效，其余情况请填写0）
	scanType：搜索类型：
					ACCURATE_VAL精确数值（value1生效）、
					LARGER_THAN_VAL值大于（value1生效）、
					LESS_THAN_VAL值小于（value1生效）、
					BETWEEN_VAL值在两值之间（value1、value2生效）
	nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
	vResultList：存放搜索完成的结果地址
	nScanAlignBytesCount：扫描的对齐字节数，默认为待搜索数值的结构大小
	pForceStopSignal: 强制中止所有任务的信号
	)
	*/
	template<typename T> static MEM_SEARCH_STATUS SearchValue(
		IMemReaderWriterProxy* pReadWriteProxy, uint64_t hProcess,
		std::shared_ptr<MemSearchSafeWorkSecWrapper> spvWaitScanMemSecList,
		T value1, T value2, float errorRange, SCAN_TYPE scanType, size_t nThreadCount,
		std::vector<ADDR_RESULT_INFO> & vResultList,
		size_t nScanAlignBytesCount = sizeof(T),
		std::atomic<bool> * pForceStopSignal = nullptr) {
		

		return Core::SearchValue(
			pReadWriteProxy, hProcess, spvWaitScanMemSecList, value1, value2, errorRange, scanType,
			nThreadCount, vResultList, nScanAlignBytesCount, pForceStopSignal); 
	}


	/*
	再次搜索内存数值
	pReadWriteProxy：读取进程内存数据的接口
	hProcess：被搜索的进程句柄
	vWaitScanMemAddr: 需要被再次搜索的内存地址列表
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
	vResultList：存放搜索完成的结果地址
	vErrorList：存放实时搜索失败的结果地址
	pForceStopSignal: 强制中止所有任务的信号
	*/
	template<typename T> static MEM_SEARCH_STATUS SearchAddrNextValue(
		IMemReaderWriterProxy* pReadWriteProxy,
		uint64_t hProcess,
		const std::vector<ADDR_RESULT_INFO> & vWaitScanMemAddr,
		T value1, T value2, float errorRange, SCAN_TYPE scanType, size_t nThreadCount,
		std::vector<ADDR_RESULT_INFO> & vResultList,
		std::vector<ADDR_RESULT_INFO> & vErrorList,
		std::atomic<bool> * pForceStopSignal = nullptr) {

		return Core::SearchAddrNextValue(
			pReadWriteProxy, hProcess, vWaitScanMemAddr,
			value1, value2, errorRange, scanType, nThreadCount,
			vResultList, vErrorList, pForceStopSignal);
	}

	/*
	内存批量搜索值在两值之间的内存地址
	pReadWriteProxy：读取进程内存数据的接口
	hProcess：被搜索的进程句柄
	spvWaitScanMemSecList: 被搜索的进程内存区域
	vBatchBetweenValue: 待搜索的批量两值之间的数值数组
	nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
	vResultList：存放搜索完成的结果地址
	nScanAlignBytesCount：扫描的对齐字节数，默认为待搜索数值的结构大小
	pForceStopSignal: 强制中止所有任务的信号
	*/
	template<typename T> static MEM_SEARCH_STATUS SearchBatchBetweenValue(
		IMemReaderWriterProxy* pReadWriteProxy,
		uint64_t hProcess,
		std::shared_ptr<MemSearchSafeWorkSecWrapper> spvWaitScanMemSecList,
		const std::vector<BATCH_BETWEEN_VAL<T>> & vBatchBetweenValue,
		size_t nThreadCount,
		std::vector<BATCH_BETWEEN_VAL_ADDR_RESULT<T>> & vResultList,
		size_t nScanAlignBytesCount = sizeof(T),
		std::atomic<bool> * pForceStopSignal = nullptr) {

		return Core::SearchBatchBetweenValue(
			pReadWriteProxy, hProcess, spvWaitScanMemSecList, vBatchBetweenValue,
			nThreadCount, vResultList, nScanAlignBytesCount, pForceStopSignal); 
	}


	/*
	内存搜索字节特征码
	pReadWriteProxy：读取进程内存数据的接口
	hProcess：被搜索的进程句柄
	spvWaitScanMemSecList: 被搜索的进程内存区域
	vFeaturesByte：字节特征码，如“char vBytes[] = {'\x68', '\x00', '\x00', '\x00', '\x40', '\x?3', '\x??', '\x7?', '\x??', '\x??', '\x??', '\x??', '\x??', '\x50', '\xE8};”
	nFeaturesByteLen：字节特征码长度
	vFuzzyBytes：模糊字节，不变动的位置用1表示，变动的位置用0表示，如“char vFuzzy[] = {'\x11', '\x11', '\x11', '\x11', '\x11', '\x01', '\x00', '\x10', '\x00', '\x00', '\x00', '\x00', '\x00', '\x11', '\x11};”
	nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
	vResultList：存放搜索完成的结果地址
	nScanAlignBytesCount：扫描的对齐字节数，默认为1
	pForceStopSignal: 强制中止所有任务的信号
	)
	*/
	static MEM_SEARCH_STATUS SearchFeaturesBytes(
		IMemReaderWriterProxy* pReadWriteProxy,
		uint64_t hProcess,
		std::shared_ptr<MemSearchSafeWorkSecWrapper> spvWaitScanMemSecList,
		const char vFeaturesByte[],
		size_t nFeaturesByteLen,
		char vFuzzyBytes[],
		size_t nThreadCount,
		std::vector<ADDR_RESULT_INFO> & vResultList,
		size_t nScanAlignBytesCount = 1,
		std::atomic<bool> * pForceStopSignal = nullptr) {
		

		return Core::SearchFeaturesBytes(
			pReadWriteProxy, hProcess, spvWaitScanMemSecList, vFeaturesByte,
			nFeaturesByteLen, vFuzzyBytes, nThreadCount, vResultList,
			nScanAlignBytesCount, pForceStopSignal); 
	}

	/*
	再次搜索内存字节特征码
	pReadWriteProxy：读取进程内存数据的接口
	hProcess：被搜索的进程句柄
	vWaitScanMemAddr: 需要再次搜索的内存地址列表
	vFeaturesByte：字节特征码，如“char vBytes[] = {0x68, 0x00, 0x00, 0x00, 0x40, 0x?3, 0x??, 0x7?, 0x??, 0x??, 0x??, 0x??, 0x??, 0x50, 0xE8};”
	nFeaturesByteLen：字节特征码长度
	vFuzzyBytes：模糊字节，不变动的位置用1表示，变动的位置用0表示，如“char vFuzzy[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11};”
	nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
	vResultList：存放搜索完成的结果地址
	vErrorList：存放实时搜索失败的结果地址
	nScanAlignBytesCount：扫描的对齐字节数，默认为1
	pForceStopSignal: 强制中止所有任务的信号
	)
	*/
	static MEM_SEARCH_STATUS SearchAddrNextFeaturesBytes(
		IMemReaderWriterProxy* pReadWriteProxy,
		uint64_t hProcess,
		const std::vector<ADDR_RESULT_INFO> & vWaitScanMemAddr,
		char vFeaturesByte[],
		size_t nFeaturesByteLen,
		char vFuzzyBytes[],
		size_t nThreadCount,
		std::vector<ADDR_RESULT_INFO> & vResultList,
		std::vector<ADDR_RESULT_INFO> & vErrorList,
		size_t nScanAlignBytesCount = 1,
		std::atomic<bool> * pForceStopSignal = nullptr) {
		

		return Core::SearchAddrNextFeaturesBytes(
			pReadWriteProxy, hProcess, vWaitScanMemAddr,
			vFeaturesByte, nFeaturesByteLen, vFuzzyBytes, nThreadCount,
			vResultList, vErrorList, nScanAlignBytesCount, pForceStopSignal); 
	}

	/*
	内存搜索文本特征码
	pReadWriteProxy：读取进程内存数据的接口
	hProcess：被搜索的进程句柄
	spvWaitScanMemSecList: 被搜索的进程内存区域
	strFeaturesByte: 十六进制的文本字节特征码，搜索规则与OD相同，如“68 00 00 00 40 ?3 7? ?? ?? ?? ?? ?? ?? 50 E8”
	nThreadCount: 用于搜索内存的线程数，推荐设置为CPU数量
	vResultList：存放搜索完成的结果地址
	nScanAlignBytesCount：扫描的对齐字节数，默认为1
	pForceStopSignal: 强制中止所有任务的信号
	)
	*/
	static MEM_SEARCH_STATUS SearchFeaturesByteString(
		IMemReaderWriterProxy* pReadWriteProxy,
		uint64_t hProcess,
		std::shared_ptr<MemSearchSafeWorkSecWrapper> spvWaitScanMemSecList,
		std::string strFeaturesByte,
		size_t nThreadCount,
		std::vector<ADDR_RESULT_INFO> & vResultList,
		size_t nScanAlignBytesCount = 1,
		std::atomic<bool> * pForceStopSignal = nullptr) {
		

		return Core::SearchFeaturesByteString(
			pReadWriteProxy, hProcess, spvWaitScanMemSecList, strFeaturesByte,
			nThreadCount, vResultList, nScanAlignBytesCount, pForceStopSignal); 
	}


	/*
	再次搜索内存文本特征码
	pReadWriteProxy：读取进程内存数据的接口
	hProcess：被搜索的进程句柄
	vWaitScanMemAddr: 需要再次搜索的内存地址列表
	strFeaturesByte: 十六进制的文本字节特征码，搜索规则与OD相同，如“68 00 00 00 40 ?3 7? ?? ?? ?? ?? ?? ?? 50 E8”
	nThreadCount: 用于搜索内存的线程数，推荐设置为CPU数量
	vResultList：存放搜索完成的结果地址
	vErrorList：存放实时搜索失败的结果地址
	nScanAlignBytesCount：扫描的对齐字节数，默认为1
	pForceStopSignal: 强制中止所有任务的信号
	)
	*/
	static MEM_SEARCH_STATUS SearchAddrNextFeaturesByteString(
		IMemReaderWriterProxy* pReadWriteProxy,
		uint64_t hProcess,
		const std::vector<ADDR_RESULT_INFO> & vWaitScanMemAddr,
		std::string strFeaturesByte,
		size_t nThreadCount,
		std::vector<ADDR_RESULT_INFO> & vResultList,
		std::vector<ADDR_RESULT_INFO> & vErrorList,
		size_t nScanAlignBytesCount = 1,
		std::atomic<bool> * pForceStopSignal = nullptr) {
		
		
		return Core::SearchAddrNextFeaturesByteString(
			pReadWriteProxy, hProcess, vWaitScanMemAddr, strFeaturesByte,
			nThreadCount, vResultList, vErrorList, nScanAlignBytesCount, pForceStopSignal); 
	}



	/*
	拷贝进程内存数据
	pReadWriteProxy：读取进程内存数据的接口
	hProcess：被搜索的进程句柄
	spvWaitScanMemSecList: 被拷贝的进程内存区域
	nThreadCount：用于拷贝内存的线程数，推荐设置为CPU数量
	std::atomic<bool> * pForceStopSignal: 强制中止所有任务的信号
	*/
	static MEM_SEARCH_STATUS CopyProcessMemData(
		IMemReaderWriterProxy* pReadWriteProxy,
		uint64_t hProcess,
		std::shared_ptr<MemSearchSafeWorkSecWrapper> spvWaitScanMemSecList,
		size_t nThreadCount,
		std::atomic<bool> * pForceStopSignal = nullptr) {

		return Core::CopyProcessMemData(
			pReadWriteProxy, hProcess, spvWaitScanMemSecList,
			nThreadCount, pForceStopSignal);
	}

	/*
	CPU多线程执行函数
	nThreadCount：线程数
	void OnThreadExecute(size_t thread_id，std::atomic<bool> *pForceStopSignal)：每条线程要执行的任务回调（线程ID号，中止信号）
	std::atomic<bool> * pForceStopSignal: 强制中止所有任务的信号
	*/
	static void MultiThreadExecOnCpu(
		size_t nThreadCount,
		std::function<void(size_t thread_id, std::atomic<bool> *pForceStopSignal)> OnThreadExecut,
		std::atomic<bool> * pForceStopSignal = nullptr) {

		Core::MultiThreadExecOnCpu(
			nThreadCount, std::move(OnThreadExecut), pForceStopSignal);
	}


}
#endif /* MEM_SEARCH_KIT_UMBRELLA_H_ */

