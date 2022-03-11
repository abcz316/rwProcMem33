#ifndef MEM_SEARCH_KIT_CORE_H_
#define MEM_SEARCH_KIT_CORE_H_
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <sstream>
#include <functional>
#include <assert.h>
#ifdef __linux__
#include <unistd.h>
#endif
#include "../../testKo/IMemReaderWriterProxy.h"
#include "MemSearchKitSafeWorkSecWrapper.h"
#include "MemSearchKitSafeVector.h"
#include "MemSearchKitSafeMap.h"
#include "MemSearchKitCompVal.h"
#include "MemSearchKitString.h"

namespace MemorySearchKit {

	struct COPY_MEM_INFO {
		uint64_t npSectionAddr = 0; //进程内存地址
		uint64_t nSectionSize = 0; //区域大小
		std::shared_ptr<unsigned char> spSaveMemBuf = nullptr; //保存到本地缓冲区的内存字节数据
	};
	struct ADDR_RESULT_INFO {
		uint64_t addr = 0; //进程内存地址
		uint64_t size = 0; //大小
		std::shared_ptr<unsigned char> spSaveData = nullptr; //保存到本地缓冲区的内存字节数据
	};

	template<typename T> struct BATCH_BETWEEN_VAL {
		T val1 = {}; //批量搜索值在两值之间的值1
		T val2 = {}; //批量搜索值在两值之间的值2
		union {
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
	template<typename T> struct BATCH_BETWEEN_VAL_ADDR_RESULT {
		ADDR_RESULT_INFO addrInfo; //结果地址
		BATCH_BETWEEN_VAL<T> originalCondition; //原始搜索条件
	};

	enum SCAN_TYPE {
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

	enum SCAN_VALUE_TYPE {
		_1 = 0,			// 1字节
		_2,				// 2字节
		_4,				// 4字节
		_8,				// 8字节
		_FLOAT = _4,	// 单精度浮点数
		_DOUBLE = _8,	// 双精度浮点数
	};

	enum MEM_SEARCH_STATUS {
		MEM_SEARCH_SUCCESS = 0, //内存搜索成功
		MEM_SEARCH_FAILED_INVALID_PARAM, //无效的参数
		MEM_SEARCH_FAILED_ALLOC_MEM, //申请内存失败
		MEM_SEARCH_FAILED_UNKNOWN, //未知错误
	};
	namespace Core {
		using namespace MemorySearchKit::CompareValue;
		using namespace MemorySearchKit::String;


		/*
		CPU多线程执行函数
		nThreadCount：线程数
		void OnThreadExecute(size_t thread_id，std::atomic<bool> *pForceStopSignal)：每条线程要执行的任务回调（线程ID号，中止信号）
		std::atomic<bool> * pForceStopSignal: 强制中止所有任务的信号
		*/
		static void MultiThreadExecOnCpu(
			size_t nThreadCount,
			std::function<void(size_t thread_id, std::atomic<bool>* pForceStopSignal)> OnThreadExecute,
			std::atomic<bool>* pForceStopSignal/* = nullptr*/) {

			std::vector<std::shared_ptr<std::mutex>> vspMtxThreadExist;
			std::vector<std::shared_ptr<std::atomic<bool>>> vbThreadStarted;
			for (int i = 0; i < nThreadCount; ++i) {
				vspMtxThreadExist.push_back(std::make_shared<std::mutex>());
				vbThreadStarted.push_back(std::make_shared<std::atomic<bool>>(false));
			}


			//开始分配线程
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
		*/
		template<typename T> static MEM_SEARCH_STATUS SearchValue(
			IMemReaderWriterProxy* pReadWriteProxy,
			uint64_t hProcess,
			std::shared_ptr<MemSearchSafeWorkSecWrapper> spvWaitScanMemSecList,
			T value1, T value2, float errorRange, SCAN_TYPE scanType, size_t nThreadCount,
			std::vector<ADDR_RESULT_INFO>& vResultList,
			size_t nScanAlignBytesCount/* = sizeof(T)*/,
			std::atomic<bool>* pForceStopSignal/* = nullptr*/) {

			assert(pReadWriteProxy);
			if (!pReadWriteProxy) {
				return MEM_SEARCH_FAILED_INVALID_PARAM;
			}

			//设置内存数据读取器
			std::shared_ptr<SimpleDriverMemDataProvider> spMemDataProvider
				= std::make_shared<SimpleDriverMemDataProvider>(pReadWriteProxy, hProcess); 
			if (!spMemDataProvider) {
				return MEM_SEARCH_FAILED_ALLOC_MEM;
			}
			spvWaitScanMemSecList->set_mem_data_provider(spMemDataProvider.get());


			//自动排序容器
			MemSearchSafeMap<uint64_t, ADDR_RESULT_INFO> sortResultMap;

			MultiThreadExecOnCpu(nThreadCount, [pReadWriteProxy, nThreadCount, hProcess,
				spvWaitScanMemSecList, value1, value2, errorRange, scanType,
				&sortResultMap, nScanAlignBytesCount](size_t thread_id, std::atomic<bool>* pForceStopSignal)->void {

					std::vector<ADDR_RESULT_INFO> vThreadOutput; //存放当前线程的搜索结果

					bool isFloatVal = typeid(T) == typeid(static_cast<float>(0)) || typeid(T) == typeid(static_cast<double>(0));

					uint64_t curMemBlockStartAddr = 0;
					uint64_t curMemBlockSize = 0;
					std::shared_ptr<std::atomic<uint64_t>> spOutCurWorkOffset;
					std::shared_ptr<unsigned char> spOutMemDataBlock;
					while (!pForceStopSignal || !*pForceStopSignal) {
						if (!spOutCurWorkOffset) {
							if (!spvWaitScanMemSecList->get_need_work_mem_sec(curMemBlockStartAddr, curMemBlockSize, spOutCurWorkOffset, spOutMemDataBlock)) {
								break; //没任务了
							}
						}
						//分配当前线程取本次工作内存的size
						size_t splitSize = curMemBlockSize / nThreadCount;
						auto yu = splitSize % max(sizeof(T), nScanAlignBytesCount);
						splitSize -= yu;

						uint64_t curWorkOffset = spOutCurWorkOffset->fetch_add(splitSize);

						if (curWorkOffset >= curMemBlockSize) {
							spOutCurWorkOffset = nullptr;
							spOutMemDataBlock = nullptr;
							continue;
						}

						//读取这个目标内存section的内存数据时直接指针取值，节省开销
						size_t nRealReadSize = min(curMemBlockSize - curWorkOffset, splitSize);
						if (nRealReadSize == 0) {
							continue;
						}
						char* pReadBuf = (char*)((char*)spOutMemDataBlock.get() + curWorkOffset);

						//寻找数值
						std::vector<size_t> vFindAddr;


						switch (scanType) {
						case SCAN_TYPE::ACCURATE_VAL:
							//精确数值
							if (isFloatVal) {
								FindBetween<T>((size_t)(pReadBuf),
									nRealReadSize, value1 - errorRange, value1 + errorRange, nScanAlignBytesCount, vFindAddr);
							}
							else {
								FindValue<T>((size_t)pReadBuf, nRealReadSize, value1, nScanAlignBytesCount, vFindAddr);
							}

							break;
						case SCAN_TYPE::LARGER_THAN_VAL:
							//值大于
							FindGreater<T>((size_t)pReadBuf, nRealReadSize, value1, nScanAlignBytesCount, vFindAddr);
							break;
						case SCAN_TYPE::LESS_THAN_VAL:
							//值小于
							FindLess<T>((size_t)pReadBuf, nRealReadSize, value1, nScanAlignBytesCount, vFindAddr);
							break;
						case SCAN_TYPE::BETWEEN_VAL:
							//值在两者之间于
							FindBetween<T>((size_t)pReadBuf, nRealReadSize, min(value1, value2), max(value1, value2), nScanAlignBytesCount, vFindAddr);
							break;
						default:
							break;
						}



						for (size_t addr : vFindAddr) {
							ADDR_RESULT_INFO aInfo;
							aInfo.addr = (uint64_t)addr - (uint64_t)pReadBuf + curMemBlockStartAddr + curWorkOffset/*每个线程的起始位置*/;
							aInfo.size = sizeof(T);

							std::shared_ptr<unsigned char> sp(new unsigned char[aInfo.size], std::default_delete<unsigned char[]>());
							memcpy(sp.get(), (void*)addr, aInfo.size);

							aInfo.spSaveData = sp;
							vThreadOutput.push_back(aInfo);
						}

					}
					//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
					for (ADDR_RESULT_INFO& newAddr : vThreadOutput) {
						sortResultMap.insert(newAddr.addr, newAddr);
					}

				}, pForceStopSignal);

			sortResultMap.to_vector(vResultList);
			return MEM_SEARCH_SUCCESS;
		}


		/*
		再次搜索内存数值
		pReadWriteProxy：读取进程内存数据的接口
		hProcess：被搜索的进程句柄
		vWaitScanMemAddr: 需要再次搜索的内存地址列表
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
			const std::vector<ADDR_RESULT_INFO>& vWaitScanMemAddr,
			T value1, T value2, float errorRange, SCAN_TYPE scanType, size_t nThreadCount,
			std::vector<ADDR_RESULT_INFO>& vResultList,
			std::vector<ADDR_RESULT_INFO>& vErrorList,
			std::atomic<bool>* pForceStopSignal/* = nullptr*/) {
			assert(pReadWriteProxy);
			if (!pReadWriteProxy) {
				return MEM_SEARCH_FAILED_INVALID_PARAM;
			}

			MemSearchSafeVector<ADDR_RESULT_INFO> vSafeWaitScanMemAddr(vWaitScanMemAddr);

			//自动排序
			MemSearchSafeMap<uint64_t, ADDR_RESULT_INFO> sortResultMap;
			MemSearchSafeMap<uint64_t, ADDR_RESULT_INFO> sortErrorMap;

			bool isFloatVal = typeid(T) == typeid(static_cast<float>(0)) || typeid(T) == typeid(static_cast<double>(0));

			//内存搜索线程
			MultiThreadExecOnCpu(nThreadCount,
				[pReadWriteProxy, hProcess, &vSafeWaitScanMemAddr, nThreadCount,
				value1, value2, errorRange, scanType, isFloatVal, &sortResultMap, &sortErrorMap](size_t thread_id, std::atomic<bool>* pForceStopSignal)->void {

					std::vector<ADDR_RESULT_INFO> vThreadOutput; //存放当前线程的搜索结果


					//一个一个job拿会拖慢速度
					std::vector<ADDR_RESULT_INFO> vTempJobMemAddrInfo;
					while (vTempJobMemAddrInfo.size() ||
						vSafeWaitScanMemAddr.pop_back(nThreadCount * 2, vTempJobMemAddrInfo) ||
						vSafeWaitScanMemAddr.pop_back(1, vTempJobMemAddrInfo)) {

						ADDR_RESULT_INFO memAddrJob = vTempJobMemAddrInfo.back();
						vTempJobMemAddrInfo.pop_back();

						T temp = 0;
						size_t dwRead = 0;
						if (!pReadWriteProxy->ReadProcessMemory(hProcess, memAddrJob.addr, &temp, sizeof(temp), &dwRead)) {
							sortErrorMap.insert(memAddrJob.addr, memAddrJob);
							continue;
						}

						if (dwRead != sizeof(T)) {
							sortErrorMap.insert(memAddrJob.addr, memAddrJob);
							continue;
						}

						//寻找数值
						switch (scanType) {
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
							if (value1 < value2) {
								if (value1 > temp || temp > value2) { continue; }
							}
							else {
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
								T* pOldData = (T*)memAddrJob.spSaveData.get();
								T cOldData = *pOldData;
								if (temp <= cOldData) { continue; }
								T add = temp - cOldData;

								if ((value1 - errorRange) > add || add > (value1 + errorRange)) { continue; }
							}
							else {
								//数值增加了的结果保留
								if (((temp)-(*(T*)(memAddrJob.spSaveData.get()))) != value1) { continue; }

							}
							break;
						case SCAN_TYPE::SUB_UNKNOW_VAL:
							//减少的数值的结果保留
							if (temp >= *(T*)(memAddrJob.spSaveData.get())) { continue; }
							break;
						case SCAN_TYPE::SUB_ACCURATE_VAL:
							if (isFloatVal) {
								//float、double数值减少了的结果保留
								T* pOldData = (T*)memAddrJob.spSaveData.get();
								T cOldData = *pOldData;
								if (temp >= cOldData) { continue; }
								T sum = cOldData - temp;

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
								if (temp > (*pOldData)) {
									if ((temp - errorRange) <= (*pOldData)) { continue; }
								}
								if (temp < (*pOldData)) {
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

								if (temp > (*pOldData)) {
									if ((temp - errorRange) > (*pOldData)) { continue; }
								}
								if (temp < (*pOldData)) {
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

						if (memAddrJob.size != sizeof(T)) {
							memAddrJob.size = sizeof(T);
							std::shared_ptr<unsigned char> sp(new unsigned char[memAddrJob.size], std::default_delete<unsigned char[]>());
							memAddrJob.spSaveData = sp;
						}
						memcpy(memAddrJob.spSaveData.get(), &temp, 1);
						vThreadOutput.push_back(memAddrJob);
					}
					//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
					for (ADDR_RESULT_INFO& newAddr : vThreadOutput) {
						sortResultMap.insert(newAddr.addr, newAddr);
					}

				}, pForceStopSignal);

			sortResultMap.to_vector(vResultList);
			sortErrorMap.to_vector(vErrorList);
			return MEM_SEARCH_SUCCESS;
		}


		/*
		内存批量搜索值在两值之间的内存地址
		pReadWriteProxy：读取进程内存数据的接口
		hProcess：被搜索的进程句柄
		spvWaitScanMemSecList: 被搜索的进程内存区域
		vBatchBetweenValue: 待搜索的多个两值之间的数值数组
		nThreadCount：用于搜索内存的线程数，推荐设置为CPU数量
		vResultList：存放搜索完成的结果地址
		nScanAlignBytesCount：扫描的对齐字节数，默认为待搜索数值的结构大小
		pForceStopSignal: 强制中止所有任务的信号
		*/
		template<typename T> static MEM_SEARCH_STATUS SearchBatchBetweenValue(
			IMemReaderWriterProxy* pReadWriteProxy,
			uint64_t hProcess,
			std::shared_ptr<MemSearchSafeWorkSecWrapper> spvWaitScanMemSecList,
			const std::vector<BATCH_BETWEEN_VAL<T>>& vBatchBetweenValue,
			size_t nThreadCount,
			std::vector<BATCH_BETWEEN_VAL_ADDR_RESULT<T>>& vResultList,
			size_t nScanAlignBytesCount/* = sizeof(T)*/,
			std::atomic<bool>* pForceStopSignal/* = nullptr*/) {

			assert(pReadWriteProxy);
			if (!pReadWriteProxy) {
				return MEM_SEARCH_FAILED_INVALID_PARAM;
			}

			//设置内存数据读取器
			std::shared_ptr<SimpleDriverMemDataProvider> spMemDataProvider
				= std::make_shared<SimpleDriverMemDataProvider>(pReadWriteProxy, hProcess); 
			if (!spMemDataProvider) {
				return MEM_SEARCH_FAILED_ALLOC_MEM;
			}
			spvWaitScanMemSecList->set_mem_data_provider(spMemDataProvider.get());



			size_t nBetweenValCount = vBatchBetweenValue.size();

			//自动排序
			MemSearchSafeMap<uint64_t, BATCH_BETWEEN_VAL_ADDR_RESULT<T>> sortResultMap;
			MultiThreadExecOnCpu(nThreadCount,
				[pReadWriteProxy, hProcess, spvWaitScanMemSecList, nThreadCount,
				&vBatchBetweenValue, nBetweenValCount, &sortResultMap, nScanAlignBytesCount]
			(size_t thread_id, std::atomic<bool>* pForceStopSignal)-> void {

					std::vector<BATCH_BETWEEN_VAL_ADDR_RESULT<T>> vThreadOutput; //存放当前线程的搜索结果

					uint64_t curMemBlockStartAddr = 0;
					uint64_t curMemBlockSize = 0;
					std::shared_ptr<std::atomic<uint64_t>> spOutCurWorkOffset;
					std::shared_ptr<unsigned char> spOutMemDataBlock;

					while (!pForceStopSignal || !*pForceStopSignal) {
						if (!spOutCurWorkOffset) {
							if (!spvWaitScanMemSecList->get_need_work_mem_sec(curMemBlockStartAddr, curMemBlockSize, spOutCurWorkOffset, spOutMemDataBlock)) {
								break; //没任务了
							}
						}
						//分配当前线程取本次工作内存的size
						size_t splitSize = curMemBlockSize / nThreadCount;
						auto yu = splitSize % max(sizeof(T), nScanAlignBytesCount);
						splitSize -= yu;

						uint64_t curWorkOffset = spOutCurWorkOffset->fetch_add(splitSize);

						if (curWorkOffset >= curMemBlockSize) {
							spOutCurWorkOffset = nullptr;
							spOutMemDataBlock = nullptr;
							continue;
						}

						//读取这个目标内存section的内存数据时直接指针取值，节省开销
						size_t nRealReadSize = min(curMemBlockSize - curWorkOffset, splitSize);
						if (nRealReadSize == 0) {
							continue;
						}
						char* pReadBuf = (char*)((char*)spOutMemDataBlock.get() + curWorkOffset);

						for (size_t i = 0; i < nBetweenValCount; i++) {
							auto& item = vBatchBetweenValue.at(i);

							//寻找数值
							std::vector<size_t> vFindAddr;

							//值在两者之间于
							FindBetween<T>((size_t)pReadBuf, nRealReadSize, min(item.val1, item.val2), max(item.val1, item.val2), nScanAlignBytesCount, vFindAddr);

							for (size_t addr : vFindAddr) {
								BATCH_BETWEEN_VAL_ADDR_RESULT<T> baInfo;
								baInfo.addrInfo.addr = (uint64_t)addr - (uint64_t)pReadBuf + curMemBlockStartAddr + curWorkOffset/*每个线程的起始位置*/;
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
					for (BATCH_BETWEEN_VAL_ADDR_RESULT<T>& newAddr : vThreadOutput) {
						sortResultMap.insert(newAddr.addrInfo.addr, newAddr);
					}
				}, pForceStopSignal);

			sortResultMap.to_vector(vResultList);
			return MEM_SEARCH_SUCCESS;
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
		*/
		static MEM_SEARCH_STATUS SearchFeaturesBytes(
			IMemReaderWriterProxy* pReadWriteProxy,
			uint64_t hProcess,
			std::shared_ptr<MemSearchSafeWorkSecWrapper> spvWaitScanMemSecList,
			const char vFeaturesByte[],
			size_t nFeaturesByteLen,
			char vFuzzyBytes[],
			size_t nThreadCount,
			std::vector<ADDR_RESULT_INFO>& vResultList,
			size_t nScanAlignBytesCount/* = 1*/,
			std::atomic<bool>* pForceStopSignal/* = nullptr*/) {

			assert(pReadWriteProxy);
			if (!pReadWriteProxy) {
				return MEM_SEARCH_FAILED_INVALID_PARAM;
			}

			//设置内存数据读取器
			std::shared_ptr<SimpleDriverMemDataProvider> spMemDataProvider 
				= std::make_shared<SimpleDriverMemDataProvider>(pReadWriteProxy, hProcess);
			if (!spMemDataProvider) {
				return MEM_SEARCH_FAILED_ALLOC_MEM;
			}
			spvWaitScanMemSecList->set_mem_data_provider(spMemDataProvider.get());


			//生成特征码容错文本
			std::string strFuzzyCode; //如：xxxxx????????xx
			for (size_t i = 0; i < nFeaturesByteLen; i++) {
				if (vFuzzyBytes[i] == '\x00') {
					strFuzzyCode += "??";
				}
				else if (vFuzzyBytes[i] == '\x01') {
					strFuzzyCode += "?x";
				}
				else if (vFuzzyBytes[i] == '\x10') {
					strFuzzyCode += "x?";
				}
				else {
					strFuzzyCode += "xx";
				}
			}

			std::shared_ptr<unsigned char> spStrFuzzyCode(new unsigned char[strFuzzyCode.length() + 1], std::default_delete<unsigned char[]>());
			memset(spStrFuzzyCode.get(), 0, strFuzzyCode.length() + 1);
			memcpy(spStrFuzzyCode.get(), strFuzzyCode.c_str(), strFuzzyCode.length());

			//是否启用特征码容错搜索
			bool isSimpleSearch = (strFuzzyCode.find("?") == -1) ? true : false;


			//自动排序
			MemSearchSafeMap<uint64_t, ADDR_RESULT_INFO> sortResultMap;

			//内存搜索线程
			MultiThreadExecOnCpu(nThreadCount,
				[pReadWriteProxy, hProcess, nThreadCount, spvWaitScanMemSecList,
				vFeaturesByte, nFeaturesByteLen, spStrFuzzyCode, isSimpleSearch,
				nScanAlignBytesCount, &sortResultMap](size_t thread_id, std::atomic<bool>* pForceStopSignal)->void {

					std::vector<ADDR_RESULT_INFO> vThreadOutput; //存放当前线程的搜索结果

					uint64_t curMemBlockStartAddr = 0;
					uint64_t curMemBlockSize = 0;
					std::shared_ptr<std::atomic<uint64_t>> spOutCurWorkOffset;
					std::shared_ptr<unsigned char> spOutMemDataBlock;
					while (!pForceStopSignal || !*pForceStopSignal) {
						if (!spOutCurWorkOffset) {
							if (!spvWaitScanMemSecList->get_need_work_mem_sec(curMemBlockStartAddr, curMemBlockSize, spOutCurWorkOffset, spOutMemDataBlock)) {
								break; //没任务了
							}
						}
						//分配当前线程取本次工作内存的size
						size_t splitSize = curMemBlockSize / nThreadCount;
						auto yu = splitSize % max(nFeaturesByteLen, nScanAlignBytesCount);
						splitSize -= yu;

						uint64_t curWorkOffset = spOutCurWorkOffset->fetch_add(splitSize);

						if (curWorkOffset >= curMemBlockSize) {
							spOutCurWorkOffset = nullptr;
							spOutMemDataBlock = nullptr;
							continue;
						}
						//读取这个目标内存section的内存数据时直接指针取值，节省开销
						size_t nRealReadSize = min((curMemBlockSize - curWorkOffset), splitSize);
						if (nRealReadSize == 0) {
							continue;
						}
						char* pReadBuf = (char*)((char*)spOutMemDataBlock.get() + curWorkOffset);


						//寻找字节集
						std::vector<size_t> vFindAddr;
						if (isSimpleSearch) {
							//不需要容错搜索
							FindBytes((size_t)pReadBuf, nRealReadSize, (unsigned char*)&vFeaturesByte[0], nFeaturesByteLen,
								nScanAlignBytesCount, vFindAddr);
						}
						else {
							//需要容错搜索
							FindFeaturesBytes((size_t)pReadBuf, nRealReadSize, (unsigned char*)&vFeaturesByte[0],
								(const char*)spStrFuzzyCode.get(), nFeaturesByteLen, nScanAlignBytesCount, vFindAddr);
						}

						for (size_t addr : vFindAddr) {
							//保存搜索结果
							ADDR_RESULT_INFO aInfo;
							aInfo.addr = (uint64_t)addr - (uint64_t)pReadBuf + curMemBlockStartAddr + curWorkOffset/*每个线程的起始位置*/;
							aInfo.size = nFeaturesByteLen;
							std::shared_ptr<unsigned char> sp(new unsigned char[aInfo.size], std::default_delete<unsigned char[]>());
							memcpy(sp.get(), (void*)addr, aInfo.size);
							aInfo.spSaveData = sp;
							vThreadOutput.push_back(aInfo);
						}
					}

					//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
					for (ADDR_RESULT_INFO& newAddr : vThreadOutput) {
						sortResultMap.insert(newAddr.addr, newAddr);
					}
				}, pForceStopSignal);
			sortResultMap.to_vector(vResultList);
			return MEM_SEARCH_SUCCESS;
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
		*/
		static MEM_SEARCH_STATUS SearchAddrNextFeaturesBytes(
			IMemReaderWriterProxy* pReadWriteProxy,
			uint64_t hProcess,
			const std::vector<ADDR_RESULT_INFO>& vWaitScanMemAddr,
			char vFeaturesByte[],
			size_t nFeaturesByteLen,
			char vFuzzyBytes[],
			size_t nThreadCount,
			std::vector<ADDR_RESULT_INFO>& vResultList,
			std::vector<ADDR_RESULT_INFO>& vErrorList,
			size_t nScanAlignBytesCount/* = 1*/,
			std::atomic<bool>* pForceStopSignal/* = nullptr*/) {

			assert(pReadWriteProxy);
			if (!pReadWriteProxy) {
				return MEM_SEARCH_FAILED_INVALID_PARAM;
			}
			//生成特征码容错文本
			std::string strFuzzyCode = ""; //如：xxxxx????????xx
			for (size_t i = 0; i < nFeaturesByteLen; i++) {
				if (vFuzzyBytes[i] == '\x00') {
					strFuzzyCode += "??";
				}
				else if (vFuzzyBytes[i] == '\x01') {
					strFuzzyCode += "?x";
				}
				else if (vFuzzyBytes[i] == '\x10') {
					strFuzzyCode += "x?";
				}
				else {
					strFuzzyCode += "xx";
				}
			}

			std::shared_ptr<unsigned char> spStrFuzzyCode(new unsigned char[strFuzzyCode.length() + 1], std::default_delete<unsigned char[]>());
			memset(spStrFuzzyCode.get(), 0, strFuzzyCode.length() + 1);
			memcpy(spStrFuzzyCode.get(), strFuzzyCode.c_str(), strFuzzyCode.length());

			//是否启用特征码容错搜索
			bool isSimpleSearch = (strFuzzyCode.find("?") == -1) ? true : false;

			MemSearchSafeVector<ADDR_RESULT_INFO> vSafeWaitScanMemAddr(vWaitScanMemAddr);

			//自动排序
			MemSearchSafeMap<uint64_t, ADDR_RESULT_INFO> sortResultMap;
			MemSearchSafeMap<uint64_t, ADDR_RESULT_INFO> sortErrorMap;

			//内存搜索线程
			MultiThreadExecOnCpu(nThreadCount,
				[pReadWriteProxy, &vSafeWaitScanMemAddr, hProcess, nThreadCount,
				vFeaturesByte, nFeaturesByteLen, spStrFuzzyCode, isSimpleSearch, nScanAlignBytesCount,
				&sortResultMap, &sortErrorMap](size_t thread_id, std::atomic<bool>* pForceStopSignal)->void {


					std::vector<ADDR_RESULT_INFO> vThreadOutput; //存放当前线程的搜索结果

					//一个一个job拿会拖慢速度
					std::vector<ADDR_RESULT_INFO> vTempJobMemAddrInfo;
					while (vTempJobMemAddrInfo.size() ||
						vSafeWaitScanMemAddr.pop_back(nThreadCount * 2, vTempJobMemAddrInfo) ||
						vSafeWaitScanMemAddr.pop_back(1, vTempJobMemAddrInfo)) {
						ADDR_RESULT_INFO memAddrJob = vTempJobMemAddrInfo.back();
						vTempJobMemAddrInfo.pop_back();

						size_t memBufSize = nFeaturesByteLen;
						unsigned char* pnew = new (std::nothrow) unsigned char[memBufSize];
						if (!pnew) {
							/*cout << "malloc "<< memSecInfo.nSectionSize  << " failed."<< endl;*/
							sortErrorMap.insert(memAddrJob.addr, memAddrJob);
							continue;
						}
						memset(pnew, 0, memBufSize);
						std::shared_ptr<unsigned char> spMemBuf(pnew, std::default_delete<unsigned char[]>());
						size_t dwRead = 0;
						if (!pReadWriteProxy->ReadProcessMemory(hProcess, memAddrJob.addr, spMemBuf.get(), memBufSize, &dwRead)) {
							sortErrorMap.insert(memAddrJob.addr, memAddrJob);
							continue;
						}
						if (dwRead != memBufSize) {
							sortErrorMap.insert(memAddrJob.addr, memAddrJob);
							continue;
						}
						//寻找字节集
						std::vector<size_t> vFindAddr;

						if (isSimpleSearch) {
							//不需要容错搜索
							FindBytes((size_t)spMemBuf.get(), dwRead, (unsigned char*)&vFeaturesByte[0], nFeaturesByteLen, nScanAlignBytesCount, vFindAddr);
						}
						else {
							//需要容错搜索
							FindFeaturesBytes((size_t)spMemBuf.get(), dwRead, (unsigned char*)&vFeaturesByte[0],
								(const char*)spStrFuzzyCode.get(), nFeaturesByteLen, nScanAlignBytesCount, vFindAddr);
						}

						if (vFindAddr.size()) {
							//保存搜索结果
							memcpy(memAddrJob.spSaveData.get(), spMemBuf.get(), memBufSize);
							vThreadOutput.push_back(memAddrJob);
						}

					}
					//将当前线程的搜索结果，汇总到父线程的全部搜索结果数组里
					for (ADDR_RESULT_INFO& newAddr : vThreadOutput) {
						sortResultMap.insert(newAddr.addr, newAddr);
					}

				}, pForceStopSignal);

			sortResultMap.to_vector(vResultList);
			sortErrorMap.to_vector(vErrorList);
			return MEM_SEARCH_SUCCESS;
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
		*/
		static MEM_SEARCH_STATUS SearchFeaturesByteString(
			IMemReaderWriterProxy* pReadWriteProxy,
			uint64_t hProcess,
			std::shared_ptr<MemSearchSafeWorkSecWrapper> spvWaitScanMemSecList,
			std::string strFeaturesByte,
			size_t nThreadCount,
			std::vector<ADDR_RESULT_INFO>& vResultList,
			size_t nScanAlignBytesCount/* = 1*/,
			std::atomic<bool>* pForceStopSignal/* = nullptr*/) {

			assert(pReadWriteProxy);
			if (!pReadWriteProxy) {
				return MEM_SEARCH_FAILED_INVALID_PARAM;
			}

			//预处理特征码
			replace_all_distinct(strFeaturesByte, " ", "");//去除空格
			if (strFeaturesByte.length() % 2) {
				return MEM_SEARCH_FAILED_INVALID_PARAM;
			}

			//特征码字节数组
			std::unique_ptr<char[]> upFeaturesBytesBuf = std::make_unique<char[]>(strFeaturesByte.length() / 2); //如：68 00 00 00 40 ?3 7? ?? ?? ?? ?? ?? ?? 50 E8

			//特征码容错数组
			std::unique_ptr<char[]> upFuzzyBytesBuf = std::make_unique<char[]>(strFeaturesByte.length() / 2);

			size_t nFeaturesBytePos = 0; //记录写入特征码字节数组的位置

			//遍历每个文本字节
			for (std::string::size_type pos(0); pos < strFeaturesByte.length(); pos += 2) {
				std::string strByte = strFeaturesByte.substr(pos, 2); //每个文本字节

				//cout << strByte <<"/" << endl;

				if (strByte == "??") //判断是不是??
				{
					upFuzzyBytesBuf[nFeaturesBytePos] = '\x00';
				}
				else if (strByte.substr(0, 1) == "?") {
					upFuzzyBytesBuf[nFeaturesBytePos] = '\x01';
				}
				else if (strByte.substr(1, 2) == "?") {
					upFuzzyBytesBuf[nFeaturesBytePos] = '\x10';
				}
				else {
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
			return SearchFeaturesBytes(
				pReadWriteProxy,
				hProcess,
				spvWaitScanMemSecList,
				upFeaturesBytesBuf.get(),
				strFeaturesByte.length() / 2,
				upFuzzyBytesBuf.get(),
				nThreadCount,
				vResultList,
				nScanAlignBytesCount,
				pForceStopSignal);

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
		*/
		static MEM_SEARCH_STATUS SearchAddrNextFeaturesByteString(
			IMemReaderWriterProxy* pReadWriteProxy,
			uint64_t hProcess,
			const std::vector<ADDR_RESULT_INFO>& vWaitScanMemAddr,
			std::string strFeaturesByte,
			size_t nThreadCount,
			std::vector<ADDR_RESULT_INFO>& vResultList,
			std::vector<ADDR_RESULT_INFO>& vErrorList,
			size_t nScanAlignBytesCount/* = 1*/,
			std::atomic<bool>* pForceStopSignal/* = nullptr*/) {

			assert(pReadWriteProxy);
			if (!pReadWriteProxy) {
				return MEM_SEARCH_FAILED_INVALID_PARAM;
			}

			//预处理特征码
			replace_all_distinct(strFeaturesByte, " ", "");//去除空格
			if (strFeaturesByte.length() % 2) {
				return MEM_SEARCH_FAILED_INVALID_PARAM;
			}

			//特征码字节数组
			std::unique_ptr<char[]> upFeaturesBytesBuf = std::make_unique<char[]>(strFeaturesByte.length() / 2); //如：68 00 00 00 40 ?3 7? ?? ?? ?? ?? ?? ?? 50 E8

			//特征码容错数组
			std::unique_ptr<char[]> upFuzzyBytesBuf = std::make_unique<char[]>(strFeaturesByte.length() / 2);

			size_t nFeaturesBytePos = 0; //记录写入特征码字节数组的位置

			//遍历每个文本字节
			for (std::string::size_type pos(0); pos < strFeaturesByte.length(); pos += 2) {
				std::string strByte = strFeaturesByte.substr(pos, 2); //每个文本字节

				if (strByte == "??") //判断是不是??
				{
					upFuzzyBytesBuf[nFeaturesBytePos] = '\x00';
				}
				else 	if (strByte.substr(0, 1) == "?") {
					upFuzzyBytesBuf[nFeaturesBytePos] = '\x01';
				}
				else 	if (strByte.substr(1, 2) == "?") {
					upFuzzyBytesBuf[nFeaturesBytePos] = '\x10';
				}
				else {
					upFuzzyBytesBuf[nFeaturesBytePos] = '\x11';
				}

				//把?替换成0
				replace_all_distinct(strByte, "?", "0");
				int nByte = 0;
				std::stringstream ssConvertBuf;
				ssConvertBuf.str(strByte);
				ssConvertBuf >> std::hex >> nByte; //十六进制字节文本转成十进制


				upFeaturesBytesBuf[nFeaturesBytePos] = nByte; //将十进制数值写进储存起来
				nFeaturesBytePos++;

			}
			return SearchAddrNextFeaturesBytes(
				pReadWriteProxy,
				hProcess,
				vWaitScanMemAddr,
				upFeaturesBytesBuf.get(),
				strFeaturesByte.length() / 2,
				upFuzzyBytesBuf.get(),
				nThreadCount,
				vResultList,
				vErrorList,
				nScanAlignBytesCount,
				pForceStopSignal);
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
			std::atomic<bool>* pForceStopSignal/* = nullptr*/) {
			assert(pReadWriteProxy);
			if (!pReadWriteProxy) {
				return MEM_SEARCH_FAILED_INVALID_PARAM;
			}

			MultiThreadExecOnCpu(nThreadCount,
				[pReadWriteProxy, hProcess, spvWaitScanMemSecList](size_t thread_id, std::atomic<bool>* pForceStopSignal)->void {
					std::vector<COPY_MEM_INFO> vThreadOutput; //存放当前线程的搜索结果

					uint64_t curMemBlockStartAddr = 0;
					uint64_t curMemBlockSize = 0;
					std::shared_ptr<std::atomic<uint64_t>> spOutCurWorkOffset;
					std::shared_ptr<unsigned char> spOutMemDataBlock;
					while (!pForceStopSignal || !*pForceStopSignal) {
						if (!spOutCurWorkOffset) {
							if (!spvWaitScanMemSecList->get_need_work_mem_sec(curMemBlockStartAddr, curMemBlockSize, spOutCurWorkOffset, spOutMemDataBlock, false)) {
								break; //没任务了
							}
						}
						spOutCurWorkOffset->fetch_add(curMemBlockSize);
						spOutCurWorkOffset = nullptr;
						spOutMemDataBlock = nullptr;
						if (pForceStopSignal && *pForceStopSignal) {
							break;
						}
					}
				}, pForceStopSignal);
			return MEM_SEARCH_SUCCESS;
		}


	}
}

#endif /* MEM_SEARCH_KIT_CORE_H_ */

