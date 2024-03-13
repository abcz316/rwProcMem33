//
// Created by abcz316 on 2020/6/25.
//

#ifndef MEM_SEARCH_KIT_SAFE_MAP_H
#define MEM_SEARCH_KIT_SAFE_MAP_H
#include <map>
#include <mutex>
#include <atomic>
#include <assert.h>
template<typename KeyT, typename ValT> struct MemSearchSafeMap {
	//清空
	void clear() {
		std::lock_guard<std::mutex> mlock(m_lockBlockAccess);
		m_mapBlock.clear();
	}
	//获取数组大小
	size_t size() {
		std::lock_guard<std::mutex> mlock(m_lockBlockAccess);
		return m_mapBlock.size();
	}
	//添加一个成员
	bool insert(const KeyT & key, const ValT & val) {
		std::lock_guard<std::mutex> mlock(m_lockBlockAccess);
		m_mapBlock[key] = val;
		return true;
	}
	//根据key获取成员对象
	bool at(const KeyT & key, ValT & out_val) {
		std::lock_guard<std::mutex> mlock(m_lockBlockAccess);
		auto iter = m_mapBlock.find(key);
		if (iter == m_mapBlock.end()) {
			return false;
		}
		out_val = iter->second;
		return true;
	}
	//删除一个成员
	bool erase(const KeyT & key) {
		std::lock_guard<std::mutex> mlock(m_lockBlockAccess);
		auto iter = m_mapBlock.find(key);
		if (iter == m_mapBlock.end()) {
			return false;
		}
		m_mapBlock.erase(iter);
		return true;
	}
	void to_vector(std::vector<ValT> & vTarget) {
		std::lock_guard<std::mutex> mlock(m_lockBlockAccess);
		for (auto iter : m_mapBlock) {
			vTarget.push_back(iter.second);
		}
	}
private:
	std::map<KeyT, ValT> m_mapBlock;
	std::mutex m_lockBlockAccess;
};


#endif //MEM_SEARCH_KIT_SAFE_MAP_H
