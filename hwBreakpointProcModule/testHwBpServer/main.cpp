
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <thread>
#include <zlib.h>
#include <sys/select.h>
#include <errno.h>
#include <elf.h>
#include <signal.h>
#include <sys/prctl.h>
#include <cinttypes>
#include "hwbpserver.h"
#include "api.h"
#include "Global.h"


//启用驱动KEY验证系统
#define CONFIG_VERIFY
#ifdef CONFIG_VERIFY
#include "../testHwBp/GetVerifyKey.h"
#endif


#define PORT 3170

//默认驱动文件名
#define DEV_FILEPATH "/dev/hwBreakpointProc1"

char versionstring[] = "HWBP Network 1.0";
ssize_t recvall(int s, void *buf, size_t size, int flags) {
	ssize_t totalreceived = 0;
	ssize_t sizeleft = size;
	unsigned char *buffer = (unsigned char*)buf;

	// enter recvall
	flags = flags | MSG_WAITALL;

	while (sizeleft > 0) {
		ssize_t i = recv(s, &buffer[totalreceived], sizeleft, flags);
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

ssize_t sendall(int s, void *buf, size_t size, int flags) {
	ssize_t totalsent = 0;
	ssize_t sizeleft = size;
	unsigned char *buffer = (unsigned char*)buf;

	while (sizeleft > 0) {
		ssize_t i = send(s, &buffer[totalsent], sizeleft, flags);

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



int DispatchCommand(int currentsocket, unsigned char command) {
	int r;
	switch (command) {
	case CMD_GETVERSION:
	{
		HwBpVersion *v;
		int versionsize = strlen(versionstring);
		v = (HwBpVersion*)malloc(sizeof(HwBpVersion) + versionsize);
		v->stringsize = versionsize;
		v->version = 1;

		memcpy((char *)v + sizeof(HwBpVersion), versionstring, versionsize);

		//version request
		sendall(currentsocket, v, sizeof(HwBpVersion) + versionsize, 0);

		free(v);

		break;
	}
	case CMD_CLOSECONNECTION:
	{
		printf("Connection %d closed properly\n", currentsocket);
		fflush(stdout);
		close(currentsocket);

		return 0;
	}

	case CMD_TERMINATESERVER:
	{
		printf("Command to terminate the server received\n");
		fflush(stdout);
		close(currentsocket);
		exit(0);
	}
	/*
	case CMD_CLOSEHANDLE:
	{
		HANDLE h;
		printf("CMD_CLOSEHANDLE\n");
		if (recvall(currentsocket, &h, sizeof(h), MSG_WAITALL) > 0)
		{
			CloseHandle(h);
			int r = 1;
			sendall(currentsocket, &r, sizeof(r), 0); //stupid naggle

		}
		else
		{
			printf("Error during read for CMD_CLOSEHANDLE\n");
			close(currentsocket);
			fflush(stdout);
			return 0;
		}
		break;
	}

	case CMD_OPENPROCESS:
	{
		int pid = 0;

		r = recvall(currentsocket, &pid, sizeof(int), MSG_WAITALL);
		if (r > 0)
		{
			int processhandle;

			printf("OpenProcess(%d)\n", pid);
			processhandle = OpenProcess(pid);

			printf("processhandle=%d\n", processhandle);
			sendall(currentsocket, &processhandle, sizeof(int), 0);
		}
		else
		{
			printf("Error\n");
			fflush(stdout);
			close(currentsocket);
			return 0;
		}
		break;
	}
	*/

	case CMD_SETHITCONDITIONS:
	{
		HIT_CONDITIONS params;

		printf("CMD_SETHITCONDITIONS\n");
		if (recvall(currentsocket, &params, sizeof(params), MSG_WAITALL) > 0) {
			int result = ProcessSetHwBpHitConditions(params);
			fflush(stdout);
			sendall(currentsocket, &result, sizeof(result), 0);
		} else {
			printf("Error during read for CMD_SETHITCONDITIONS\n");
			fflush(stdout);
			close(currentsocket);
			return 0;
		}

		break;
	}
	case CMD_ADDPROCESSHWBP:
	{
		AddProcessHwBpInfo params;

		printf("CMD_ADDPROCESSHWBP\n");
		if (recvall(currentsocket, &params, sizeof(AddProcessHwBpInfo), MSG_WAITALL) > 0) {
			printf("pid:%d,addr:%p,len:%d,type:%d,thread:%d,time:%d\n",
				params.pid,
				params.address,
				params.hwBpAddrLen,
				params.hwBpAddrType,
				params.hwBpThreadType,
				params.hwBpKeepTimeMs);

			int allTaskCount = 0;
			int insHwBpSuccessTaskCount = 0;
			std::vector<USER_HIT_INFO> vHit;
			ProcessAddProcessHwBp(params, allTaskCount, insHwBpSuccessTaskCount, vHit);
			fflush(stdout);

#pragma pack(1)
			struct {
				uint32_t allTaskCount;
				uint32_t insHwBpSuccessTaskCount;
				uint32_t hitCount;
			} buf;
#pragma pack()
			buf.allTaskCount = allTaskCount;
			buf.insHwBpSuccessTaskCount = insHwBpSuccessTaskCount;
			buf.hitCount = vHit.size();
			sendall(currentsocket, &buf, sizeof(buf), 0);

			for (USER_HIT_INFO h : vHit) {
				sendall(currentsocket, &h, sizeof(h), 0);
			}
		} else {
			printf("Error during read for CMD_ADDPROCESSHWBP\n");
			fflush(stdout);
			close(currentsocket);
			return 0;
		}
		break;
	}

	default:
		printf("Unknow command:%d", command);
		fflush(stdout);
		break;
	}
	return 0;
}



void newConnectionThread(int s) {
	unsigned char command;

	int currentsocket = s;

	//printf("new connection. Using socket %d\n", s);
	while (1) {
		int r = recvall(currentsocket, &command, 1, MSG_WAITALL);

		if (r > 0) {
			DispatchCommand(currentsocket, command);
		} else
			if (r == -1) {
				printf("read error on socket %d (%d)\n", s, errno);
				fflush(stdout);
				close(currentsocket);
				return;
			} else {
				if (r == 0) {
					printf("Peer has disconnected\n");
					fflush(stdout);
					close(currentsocket);
					return;
				}
			}
	}
	close(s);
	return;
}

void IdentifierThread() {
	int i;
	int s;
	int v = 1;
#pragma pack(1)
	struct {
		uint32_t checksum;
		uint16_t port;
	} packet;
#pragma pack()

	socklen_t clisize;
	struct sockaddr_in addr, addr_client;

	printf("IdentifierThread active\n");

	fflush(stdout);

	s = socket(PF_INET, SOCK_DGRAM, 0);
	i = setsockopt(s, SOL_SOCKET, SO_BROADCAST, &v, sizeof(v));

	memset(&addr, 0, sizeof(addr));

	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(3290);
	i = bind(s, (struct sockaddr *)&addr, sizeof(addr));

	if (i >= 0) {
		while (1) {
			memset(&addr_client, 0, sizeof(addr_client));
			addr_client.sin_family = PF_INET;
			addr_client.sin_addr.s_addr = INADDR_ANY;
			addr_client.sin_port = htons(3290);

			clisize = sizeof(addr_client);

			i = recvfrom(s, &packet, sizeof(packet), 0, (struct sockaddr *)&addr_client, &clisize);

			//i=recv(s, &v, sizeof(v), 0);
			if (i >= 0) {

				printf("Identifier thread received a message :%d\n", v);
				printf("sizeof(packet)=%ld\n", sizeof(packet));

				printf("packet.checksum=%x\n", packet.checksum);
				packet.checksum *= 0xce;
				packet.port = PORT;
				printf("packet.checksum=%x\n", packet.checksum);

				// packet.checksum=00AE98E7 - y=8C7F09E2

				fflush(stdout);

				i = sendto(s, &packet, sizeof(packet), 0, (struct sockaddr *)&addr_client, clisize);
				printf("sendto returned %d\n", i);
			} else {
				printf("recvfrom failed\n");
			}

			fflush(stdout);
		}
	} else {
		printf("IdentifierThread bind port failed\n");
	}
	printf("IdentifierThread exit\n");
	return;
}


int main(int argc, char *argv[]) {
	printf("Connecting driver.\n");

	//连接驱动
	int err = 0;
	if (!g_Driver.ConnectDriver(err)) {
		printf("Connect driver failed.\n");
		fflush(stdout);
		return 0;
	}


#ifdef CONFIG_VERIFY
	//驱动_设置密匙（如果驱动开启了密匙验证系统，则需要输入密匙才可读取其他进程内存，否则只能读取自身进程）
	char identityBuf128[128] = { 0 };
	if (g_Driver.SetKey((char*)&identityBuf128, 0)) {
		printf("Driver verify identity:%" PRIu64 "\n", *(uint64_t*)&identityBuf128);

		uint64_t key = GetVerifyKey((char*)&identityBuf128, sizeof(identityBuf128));
		printf("Driver verify key:%" PRIu64 "\n", key);

		if (g_Driver.SetKey(NULL, key)) {
			printf("Driver verify key success.\n");
		} else {
			printf("Driver verify key failed.\n");
		}
	} else {
		printf("Get driver verify question failed.\n");
	}
#endif



	socklen_t clisize;
	struct sockaddr_in addr, addr_client;

	printf("listening on port %d\n", PORT);

	printf("HWServer. Waiting for client connection\n");

	//if (broadcast)

	std::thread tdId(IdentifierThread);
	tdId.detach();

	int s = socket(AF_INET, SOCK_STREAM, 0);
	printf("socket=%d\n", s);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	int optval = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); //让端口释放后立即就可以被再次使用

	int b = bind(s, (struct sockaddr *)&addr, sizeof(addr));
	printf("bind=%d\n", b);


	if (b != -1) {
		int l = listen(s, 32);

		printf("listen=%d\n", l);

		clisize = sizeof(addr_client);
		memset(&addr_client, 0, sizeof(addr_client));

		fflush(stdout);

		while (1) {

			int a = accept(s, (struct sockaddr *)&addr_client, &clisize);

			printf("accept=%d\n", a);

			fflush(stdout);
			int opt = 1;
			setsockopt(a, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

			if (a != -1) {
				std::thread tdConnect(newConnectionThread, a);
				tdConnect.detach();
			}
		}
	}

	printf("Terminate server\n");

	close(s);

	return 0;
}