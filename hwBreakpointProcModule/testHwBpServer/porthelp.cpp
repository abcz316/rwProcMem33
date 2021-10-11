#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <random>
#include "porthelp.h"
#include "Global.h"
/*
HANDLE CreateHandleFromPointer(uint64_t p, handleType type)
{
	std::random_device rd;
	HANDLE handle = 10000 + rd() / 1000;
	handle = handle < 0 ? -handle : handle;
	while (g_HandlePointList.find(handle) != g_HandlePointList.end())
	{
		handle = 10000 + rd() / 1000;
		handle = handle < 0 ? -handle : handle;
	}

	HANDLE_INFO hinfo = { 0 };
	hinfo.p = p;
	hinfo.type = type;
	g_HandlePointList.insert(
		std::pair<HANDLE, HANDLE_INFO >(handle, hinfo));

	return handle;
}

handleType GetHandleType(HANDLE handle)
{
	auto item = g_HandlePointList.find(handle);
	if (item == g_HandlePointList.end())
	{
		return htEmpty;
	}
	return item->second.type;
}

uint64_t GetPointerFromHandle(HANDLE handle)
{
	auto item = g_HandlePointList.find(handle);
	if (item == g_HandlePointList.end())
	{
		return 0;
	}
	return item->second.p;
}

*/