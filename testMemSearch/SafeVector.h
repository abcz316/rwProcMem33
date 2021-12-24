//
// Created by abcz316 on 2020/6/25.
//

#ifndef SAFE_VECTOR_H
#define SAFE_VECTOR_H
#include <vector>
#include <mutex>
#include <algorithm>
#include <atomic>
template<typename T> struct SafeVector
{
	SafeVector() {}
	SafeVector(std::vector<T> & vPool) { init(vPool); }
	void init(std::vector<T> & vPool)
	{
		std::lock_guard<std::mutex> mlock(m_taskPoolLock);
		m_vPool.clear();
		m_vPool.assign(vPool.begin(), vPool.end());
		return;
	}
	void clear()
	{
		std::lock_guard<std::mutex> mlock(m_taskPoolLock);
		m_vPool.clear();
		return;
	}
	size_t size()
	{
		return m_nTaskCnt;
	}

	bool push_back(T & in)
	{
		std::lock_guard<std::mutex> mlock(m_taskPoolLock);
		m_vPool.push_back(in);
		m_nTaskCnt++;
		return true;
	}

	bool get_val(T & out)
	{
		std::lock_guard<std::mutex> mlock(m_taskPoolLock);
		if (!m_vPool.size())
		{
			return false;
		}
		out = m_vPool.back();
		m_vPool.pop_back();
		m_nTaskCnt--;
		return true;
	}

	const T & at(size_t i)
	{
		std::lock_guard<std::mutex> mlock(m_taskPoolLock);
		return m_vPool.at(i);
	}

	void sort(bool func(const T & a, const T & b))
	{
		std::lock_guard<std::mutex> mlock(m_taskPoolLock);
		std::sort(m_vPool.begin(), m_vPool.end(), func);
	}
private:
	std::vector<T> m_vPool;
	std::atomic<size_t> m_nTaskCnt{0};
	std::mutex m_taskPoolLock;
};


#endif //SAFE_VECTOR_H
