#ifndef MEM_SEARCH_KIT_SAFE_WORK_BLOCK_WRAPPER_H_
#define MEM_SEARCH_KIT_SAFE_WORK_BLOCK_WRAPPER_H_
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <assert.h>
#include "../../../testKo/jni/IMemReaderWriterProxy.h"

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

class MemSearchSafeWorkBlockWrapper : public std::enable_shared_from_this<MemSearchSafeWorkBlockWrapper> {
public:
	struct MemWorkBlock {
		uint64_t startAddr = 0;
		std::shared_ptr<unsigned char> spMemData;
		uint64_t originMemDataSize = 0;

		uint64_t workLimitSize = 0;

		std::shared_ptr<std::atomic<uint64_t>> spCurWorkOffset;
		uint64_t originWorkOffset = 0;
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
		MemWorkBlock oNewWorkBlock;
		oNewWorkBlock.startAddr = startAddr;
		oNewWorkBlock.workLimitSize = workLimitSize;
		oNewWorkBlock.spCurWorkOffset = std::make_shared<std::atomic<uint64_t>>(originWorkOffset);
		oNewWorkBlock.originWorkOffset = originWorkOffset;
		oNewWorkBlock.originMemDataSize = originMemDataSize;
		m_vNormalMemBlock.push_back(oNewWorkBlock);
		set_mem_data_provider(pMemDataProvider);
		return true;
	}
	bool push_back(const MemWorkBlock & block, IMemDataProvider *pMemDataProvider = nullptr) {
		std::lock_guard<std::mutex> mlock(m_lockAccess);
		m_vNormalMemBlock.push_back(block);
		set_mem_data_provider(pMemDataProvider);
		return true;
	}

	//释放无用的内存
	void release_useless_mem_block() {
		std::lock_guard<std::mutex> mlock(m_lockAccess);
		assert(m_pMemDataProvider);
		if (!m_pMemDataProvider) {
			return;
		}
		for (auto i = 0; i < normal_block_count(); i++) {
			auto & block = m_vNormalMemBlock[i];
			if (block.spCurWorkOffset->load() >= block.workLimitSize) {
				block.spMemData = nullptr;
			}
		}
	}

	//获取一个需要工作的内存区域
	bool get_need_work_mem_block(size_t canWorkSize, uint64_t & outMemStartAddr, uint64_t & outMemSize,
		std::shared_ptr<unsigned char> & spOutMemData,
		uint64_t & outCurWorkOffset) {
		std::lock_guard<std::mutex> mlock(m_lockAccess);
		assert(m_pMemDataProvider);
		if (!m_pMemDataProvider) {
			return false;
		}
		for (auto i = 0; i < normal_block_count(); i++) {
			auto & block = m_vNormalMemBlock[i];
			if (block.spCurWorkOffset->load() >= block.workLimitSize) {
				continue;
			}
			//开始读内存
			if (!block.spMemData) {
				size_t nNumberOfBytesRead = 0;
				std::shared_ptr<unsigned char> spMem(new unsigned char[block.workLimitSize], std::default_delete<unsigned char[]>());
				if (!m_pMemDataProvider->ReadMemory(block.startAddr, spMem.get(), block.workLimitSize, &nNumberOfBytesRead) ||
					nNumberOfBytesRead != block.workLimitSize) {
					spMem = nullptr;
					//读内存失败了，先跳过这段内存这样处理吧
					block.spCurWorkOffset->store(block.workLimitSize);
					m_vErrorMemBlock.push_back(block); //记录起来
					continue;
				}
				block.spMemData = spMem;
			}
			outCurWorkOffset = block.spCurWorkOffset->fetch_add(canWorkSize);
			outMemStartAddr = block.startAddr;
			outMemSize = block.workLimitSize;
			spOutMemData = block.spMemData;
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

		std::vector<MemWorkBlock> vNormalMemBlock(m_vNormalMemBlock);
		IMemDataProvider * pMemDataProvider = m_pMemDataProvider;
		_clean();
		m_pMemDataProvider = pMemDataProvider;
		m_vNormalMemBlock.assign(vNormalMemBlock.begin(), vNormalMemBlock.end());
		for (auto & item : m_vNormalMemBlock) {
			if (item.spCurWorkOffset) {
				item.spCurWorkOffset->store(item.originWorkOffset);
			}
		}
	}
	//获取内存块数量
	size_t normal_block_count() {
		return m_vNormalMemBlock.size();
	}
	//获取内存块工作失败数量
	size_t error_block_count() {
		return m_vErrorMemBlock.size();
	}
	//拷贝内存块
	void copy_normal_block_to(std::vector<MemWorkBlock> & vMemBlock) {
		std::lock_guard<std::mutex> mlock(m_lockAccess);
		vMemBlock = m_vNormalMemBlock;
	}
	void copy_normal_block_to(MemSearchSafeWorkBlockWrapper & targetWorkBlockWrapper, bool copy_new_cur_work_offset = true, bool copy_new_mem_data = true) {
		std::vector<MemWorkBlock> vMemBlock;
		copy_normal_block_to(vMemBlock);
		targetWorkBlockWrapper.clean();
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
			targetWorkBlockWrapper.push_back(item);
		}
		targetWorkBlockWrapper.set_mem_data_provider(get_mem_data_provider());
	}

	//拷贝工作失败的内存块
	void copy_error_block_to(std::vector<MemWorkBlock> & vErrorMemBlock) {
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
		m_vNormalMemBlock.clear();
		m_vErrorMemBlock.clear();
		m_pMemDataProvider = nullptr;
	}

private:
	std::mutex m_lockAccess;

	std::vector<MemWorkBlock> m_vNormalMemBlock;
	std::vector<MemWorkBlock> m_vErrorMemBlock;
	
	std::atomic<size_t> m_nFastNormalMemBlockCount{ 0 };
	std::atomic<size_t> m_nFastErrorMemBlockCount{ 0 };

	IMemDataProvider *m_pMemDataProvider = nullptr;
};
#endif /* MEM_SEARCH_KIT_SAFE_WORK_BLOCK_WRAPPER_H_ */

