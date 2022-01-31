#ifndef GLOBAL_H_
#define GLOBAL_H_
#include <map>
#include <vector>
#include <memory>
#include <atomic>
#include <cstdint>
#include <cinttypes>
#include <WinSock2.h>
#pragma comment(lib, "comsuppw.lib")
#include "NetworkManager.h"

extern CNetworkManager g_NetworkManager;

static std::string ws2s(const std::wstring& ws) {
	_bstr_t t = ws.c_str();
	char* pchar = (char*)t;
	std::string result = pchar;
	return result;
}

static std::wstring s2ws(const std::string& s) {
	_bstr_t t = s.c_str();
	wchar_t* pwchar = (wchar_t*)t;
	std::wstring result = pwchar;
	return result;
}
#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif
static ssize_t recvall(SOCKET s, void *buf, size_t size, int flags) {
	ssize_t totalreceived = 0;
	ssize_t sizeleft = size;
	unsigned char *buffer = (unsigned char*)buf;

	// enter recvall
	flags = flags | MSG_WAITALL;

	while (sizeleft > 0) {
		auto i = recv(s, (char*)&buffer[totalreceived], sizeleft, flags);
		if (i == 0) {
			printf("recv returned 0\n");
			return i;
		}
		if (i == -1) {
			printf("recv returned -1\n");
			if (errno == EINTR) {
				printf("errno = EINTR\n");
				i = 0;
			} else {
				printf("Error during recvall: %d. errno=%d\n", (int)i, errno);
				return i; //read error, or disconnected
			}
		}
		totalreceived += i;
		sizeleft -= i;
	}
	// leave recvall
	return totalreceived;
}

static ssize_t sendall(SOCKET s, void *buf, size_t size, int flags) {
	ssize_t totalsent = 0;
	ssize_t sizeleft = size;
	unsigned char *buffer = (unsigned char*)buf;

	while (sizeleft > 0) {
		auto i = send(s, (const char*)&buffer[totalsent], sizeleft, flags);

		if (i == 0) {
			return i;
		}

		if (i == -1) {
			if (errno == EINTR) {
				i = 0;
			} else {
				printf("Error during sendall: %d. errno=%d\n", (int)i, errno);
				return i;
			}
		}
		totalsent += i;
		sizeleft -= i;
	}

	return totalsent;
}


#endif /* GLOBAL_H_ */