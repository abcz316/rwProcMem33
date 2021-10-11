#pragma once
#include <map>
#include <vector>
#include <memory>
#include <atomic>
#include "api.h"
#include "../testHwBp/HwBreakpointManager.h"
/*
struct HANDLE_INFO
{
	uint64_t p;
	handleType type;


};
extern std::map<HANDLE, HANDLE_INFO> g_HandlePointList;
*/
extern CHwBreakpointManager g_Driver;

