#include "porthelp.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <random>
#include <map>
#include "api.h"

struct HANDLE_INFO {
	uint64_t p;//TODO：这里有空再搞成shared_ptr派生成各个基类，防止内存泄漏
	handleType type;
};
std::map<HANDLE, HANDLE_INFO> m_HandlePointList;


HANDLE CPortHelper::CreateHandleFromPointer(uint64_t p, handleType type) {
	std::random_device rd;
	HANDLE handle = 10000 + rd() / 1000;
	handle = handle < 0 ? -handle : handle;
	while (m_HandlePointList.find(handle) != m_HandlePointList.end()) {
		handle = 10000 + rd() / 1000;
		handle = handle < 0 ? -handle : handle;
	}

	HANDLE_INFO hinfo = { 0 };
	hinfo.p = p;
	hinfo.type = type;
	m_HandlePointList.insert(
		std::pair<HANDLE, HANDLE_INFO >(handle, hinfo));

	return handle;
}

handleType CPortHelper::GetHandleType(HANDLE handle) {
	auto iter = m_HandlePointList.find(handle);
	if (iter == m_HandlePointList.end()) {
		return htEmpty;
	}
	return iter->second.type;
}

uint64_t CPortHelper::GetPointerFromHandle(HANDLE handle) {
	auto iter = m_HandlePointList.find(handle);
	if (iter == m_HandlePointList.end()) {
		return 0;
	}
	return iter->second.p;
}



void CPortHelper::RemoveHandle(HANDLE handle) {
	auto iter = m_HandlePointList.find(handle);
	if (iter == m_HandlePointList.end()) {
		return;
	}
	m_HandlePointList.erase(iter);
}



HANDLE CPortHelper::FindHandleByPID(DWORD pid) {
	for (auto item = m_HandlePointList.begin(); item != m_HandlePointList.end(); item++) {
		if (item->second.type == htProcesHandle) {
			CeOpenProcess *pCeOpenProcess = (CeOpenProcess*)item->second.p;
			if (pCeOpenProcess->pid == pid) {
				return item->first;
			}
		}
	}
	return 0;
}
