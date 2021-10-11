
#ifndef PORTHELP_H_
#define PORTHELP_H_
#include <stdint.h>

typedef int32_t HANDLE; //just an int, in case of a 32-bit ce version and a 64-bit linux version I can not give pointers, so use ID's for handles
typedef int32_t DWORD;

typedef enum { htEmpty = 0, htProcesHandle} handleType; //The difference between ThreadHandle and NativeThreadHandle is that threadhandle is based on the processid of the thread, the NativeThreadHandle is in linux usually the pthread_t handle
typedef int BOOL;


/*
HANDLE CreateHandleFromPointer(uint64_t p, handleType type);
handleType GetHandleType(HANDLE handle);
uint64_t GetPointerFromHandle(HANDLE handle);
*/
#define TRUE 1
#define FALSE 0

#endif /* PORTHELP_H_ */
