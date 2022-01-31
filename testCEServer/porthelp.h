#pragma once
#include <stdint.h>

typedef int32_t HANDLE; //just an int, in case of a 32-bit ce version and a 64-bit linux version I can not give pointers, so use ID's for handles
typedef int32_t DWORD;

#define TH32CS_SNAPPROCESS  0x2
#define TH32CS_SNAPMODULE   0x8

#define PAGE_NOACCESS 1
#define PAGE_READONLY 2
#define PAGE_READWRITE 4
#define PAGE_WRITECOPY 8
#define PAGE_EXECUTE 16
#define PAGE_EXECUTE_READ 32
#define PAGE_EXECUTE_READWRITE 64

#define MEM_MAPPED 262144
#define MEM_PRIVATE 131072

#define TRUE 1
#define FALSE 0

typedef enum {
	htEmpty = 0,
	htProcesHandle,
	htThreadHandle,
	htTHSProcess,
	htTHSModule,
	htNativeThreadHandle
} handleType; //The difference between ThreadHandle and NativeThreadHandle is that threadhandle is based on the processid of the thread, the NativeThreadHandle is in linux usually the pthread_t handle

typedef int BOOL;


class CPortHelper {
public:
	static HANDLE CreateHandleFromPointer(uint64_t p, handleType type);
	static handleType GetHandleType(HANDLE handle);
	static uint64_t GetPointerFromHandle(HANDLE handle);
	static void RemoveHandle(HANDLE handle);
	static HANDLE FindHandleByPID(DWORD pid);
};

