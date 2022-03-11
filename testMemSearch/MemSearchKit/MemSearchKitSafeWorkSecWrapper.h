#ifndef MEM_SEARCH_KIT_SAFE_WORK_SEC_WRAPPER_H_
#define MEM_SEARCH_KIT_SAFE_WORK_SEC_WRAPPER_H_
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <assert.h>
#include "../../testKo/IMemReaderWriterProxy.h"

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
struct IMemDataProvider {
	virtual BOOL ReadMemory(
		uint64_t lpBaseAddress,
		void *lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesRead) = 0;
};

class SimpleDriverMemDataProvider : public IMemDataProvider {
public:
	SimpleDriverMemDataProvider(IMemReaderWriterProxy * pReadWriteProxy, uint64_t hProcess)
		: m_pReadWriteProxy(pReadWriteProxy), m_hProcess(hProcess) {

	}
protected:
	BOOL ReadMemory(
		uint64_t lpBaseAddress,
		void *lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesRead) override {
		if (!m_pReadWriteProxy) {
			return FALSE;
		}
		return m_pReadWriteProxy->ReadProcessMemory(m_hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
	}
private:
	IMemReaderWriterProxy * m_pReadWriteProxy = nullptr;
	uint64_t m_hProcess = 0;
};

class MemSearchSafeWorkSecWrapper : public std::enable_shared_from_this<MemSearchSafeWorkSecWrapper> {
public:
	struct WorkMemSecBlock {
		uint64_t startAddr = 0;
		uint64_t workLimitSize = 0;
		std::shared_ptr<std::atomic<uint64_t>> spCurWorkOffset;
		uint64_t originWorkOffset = 0;
		std::shared_ptr<unsigned char> spMemData;
		uint64_t originMemDataSize = 0;
	};

	//设置内存数据提供器
	void set_mem_data_provider(IMemDataProvider *pMemDataProvider = nullptr) {
		m_pMemDataProvider = pMemDataProvider;
	}
	//获取内存数据提供器
	IMemDataProvider * get_mem_data_provider() {
		return m_pMemDataProvider;
	}

	//添加一个可工作的内存区域
	bool push_back(uint64_t startAddr, uint64_t workLimitSize, uint64_t originWorkOffset,
		uint64_t originMemDataSize, IMemDataProvider *pMemDataProvider = nullptr) {

		std::lock_guard<std::mutex> mlock(m_lockAccess);
		WorkMemSecBlock oNewWorkSecBlock;
		oNewWorkSecBlock.startAddr = startAddr;
		oNewWorkSecBlock.workLimitSize = workLimitSize;
		oNewWorkSecBlock.spCurWorkOffset = std::make_shared<std::atomic<uint64_t>>(originWorkOffset);
		oNewWorkSecBlock.originWorkOffset = originWorkOffset;
		oNewWorkSecBlock.originMemDataSize = originMemDataSize;
		m_vNormalMemBlock.push_back(oNewWorkSecBlock);
		set_mem_data_provider(pMemDataProvider);
		m_nFastNormalMemBlockCount++;
		return true;
	}
	bool push_back(const WorkMemSecBlock & block, IMemDataProvider *pMemDataProvider = nullptr) {
		std::lock_guard<std::mutex> mlock(m_lockAccess);
		m_vNormalMemBlock.push_back(block);
		set_mem_data_provider(pMemDataProvider);
		m_nFastNormalMemBlockCount++;
		return true;
	}

	//获取一个需要工作的内存区域
	bool get_need_work_mem_sec(uint64_t & outMemStartAddr, uint64_t & outMemSize,
		std::shared_ptr<std::atomic<uint64_t>> & spOutCurWorkOffset,
		std::shared_ptr<unsigned char> & spOutMemDataBlock,
		bool releaseUselessBlockMemData = true) {
		std::lock_guard<std::mutex> mlock(m_lockAccess);
		assert(m_pMemDataProvider);
		if (!m_pMemDataProvider) {
			return false;
		}
		for (; m_nNormalMemIndex < normal_block_count(); m_nNormalMemIndex++) {
			auto & sec = m_vNormalMemBlock[m_nNormalMemIndex];
			if (sec.spCurWorkOffset->load() >= sec.workLimitSize) {
				if (releaseUselessBlockMemData) { //释放无用的内存
					sec.spMemData = nullptr;
				}
				continue;
			}
			//开始读内存
			if (!sec.spMemData) {
				size_t nNumberOfBytesRead = 0;
				std::shared_ptr<unsigned char> spMem(new unsigned char[sec.workLimitSize], std::default_delete<unsigned char[]>());

				if (!m_pMemDataProvider->ReadMemory(sec.startAddr, spMem.get(), sec.workLimitSize, &nNumberOfBytesRead) ||
					nNumberOfBytesRead != sec.workLimitSize) {
					//读内存失败了，先跳过这段内存这样处理吧
					sec.spCurWorkOffset->store(sec.workLimitSize);
					m_vErrorMemBlock.push_back(sec); //记录起来
					continue;
				}
				sec.spMemData = spMem;
			}
			spOutCurWorkOffset = sec.spCurWorkOffset;
			outMemStartAddr = sec.startAddr;
			outMemSize = sec.workLimitSize;
			spOutMemDataBlock = sec.spMemData;
			return true;

		}
		return false;
	}
	//清空全部
	void clean() {
		std::lock_guard<std::mutex> mlock(m_lockAccess);
		_clean();
	}
	//恢复内存块初始进度
	void recover_normal_block_origin_progress() {
		std::lock_guard<std::mutex> mlock(m_lockAccess);

		std::vector<WorkMemSecBlock> vNormalMemBlock(m_vNormalMemBlock);
		IMemDataProvider * pMemDataProvider = m_pMemDataProvider;
		_clean();
		m_pMemDataProvider = pMemDataProvider;
		m_vNormalMemBlock.assign(vNormalMemBlock.begin(), vNormalMemBlock.end());
		m_nFastNormalMemBlockCount = m_vNormalMemBlock.size();
		for (auto & item : m_vNormalMemBlock) {
			if (item.spCurWorkOffset) {
				item.spCurWorkOffset->store(item.originWorkOffset);
			}
		}
	}
	//获取内存块数量
	size_t normal_block_count() {
		return m_nFastNormalMemBlockCount;
	}
	//获取内存块工作失败数量
	size_t error_block_count() {
		return m_nFastErrorMemBlockCount;
	}
	//拷贝内存块
	void copy_normal_block_to(std::vector<WorkMemSecBlock> & vMemBlock) {
		std::lock_guard<std::mutex> mlock(m_lockAccess);
		vMemBlock = m_vNormalMemBlock;
	}
	void copy_normal_block_to(MemSearchSafeWorkSecWrapper & targetWorkSecWrapper, bool copy_new_cur_work_offset = true, bool copy_new_mem_data = true) {
		std::vector<WorkMemSecBlock> vMemBlock;
		copy_normal_block_to(vMemBlock);
		targetWorkSecWrapper.clean();
		for (auto & item : vMemBlock) {
			if (copy_new_cur_work_offset && item.spCurWorkOffset) {
				std::shared_ptr<std::atomic<uint64_t>> spNewOffset = std::make_shared<std::atomic<uint64_t>>(0);
				spNewOffset->store(item.spCurWorkOffset->load());
				item.spCurWorkOffset = spNewOffset;
			}
			if (copy_new_mem_data && item.spMemData) {
				std::shared_ptr<unsigned char> spNewMem(new unsigned char[item.originMemDataSize], std::default_delete<unsigned char[]>());
				memcpy(spNewMem.get(), item.spMemData.get(), item.originMemDataSize);
				item.spMemData = spNewMem;
			}
			targetWorkSecWrapper.push_back(item);
		}
		targetWorkSecWrapper.set_mem_data_provider(get_mem_data_provider());
	}

	//拷贝工作失败的内存块
	void copy_error_block_to(std::vector<WorkMemSecBlock> & vErrorMemBlock) {
		std::lock_guard<std::mutex> mlock(m_lockAccess);
		vErrorMemBlock = m_vErrorMemBlock;
	}
	//获取内存总大小
	uint64_t get_mem_total_size() {
		std::lock_guard<std::mutex> mlock(m_lockAccess);
		uint64_t total = 0;
		for (auto & item : m_vNormalMemBlock) {
			total += item.originMemDataSize;
		}
		return total;
	}
	//获取剩余未工作的内存大小
	uint64_t get_mem_remaining_size() {
		std::lock_guard<std::mutex> mlock(m_lockAccess);
		uint64_t remainingMemSize = 0;
		for (auto & item : m_vNormalMemBlock) {
			remainingMemSize += item.workLimitSize - min(item.workLimitSize, item.spCurWorkOffset->load());
		}
		return remainingMemSize;
	}
private:
	void _clean() {
		m_nNormalMemIndex = 0;
		m_vNormalMemBlock.clear();
		m_vErrorMemBlock.clear();
		m_nFastNormalMemBlockCount = 0;
		m_nFastErrorMemBlockCount = 0;
		m_pMemDataProvider = nullptr;
	}

private:
	std::mutex m_lockAccess;

	int m_nNormalMemIndex = 0;
	std::vector<WorkMemSecBlock> m_vNormalMemBlock;
	std::vector<WorkMemSecBlock> m_vErrorMemBlock;
	
	std::atomic<size_t> m_nFastNormalMemBlockCount{ 0 };
	std::atomic<size_t> m_nFastErrorMemBlockCount{ 0 };

	IMemDataProvider *m_pMemDataProvider = nullptr;
};
#endif /* MEM_SEARCH_KIT_SAFE_WORK_SEC_WRAPPER_H_ */

