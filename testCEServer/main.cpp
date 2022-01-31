
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
#include <vector>
#include "ceserver.h"
#include "api.h"
#include "context.h"
#include "native-api.h"
#define PORT 3168


char versionstring[] = "CHEATENGINE Network 2.0";
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
		CeVersion *v;
		int versionsize = strlen(versionstring);
		v = (CeVersion*)malloc(sizeof(CeVersion) + versionsize);
		v->stringsize = versionsize;
		v->version = 1;

		memcpy((char *)v + sizeof(CeVersion), versionstring, versionsize);

		//version request
		sendall(currentsocket, v, sizeof(CeVersion) + versionsize, 0);

		free(v);

		break;
	}

	case CMD_GETARCHITECTURE:
	{
#ifdef __i386__
		unsigned char arch = 0;
#endif
#ifdef __x86_64__
		unsigned char arch = 1;
#endif
#ifdef __arm__
		unsigned char arch = 2;
#endif
#ifdef __aarch64__
		unsigned char arch = 3;
#endif

		sendall(currentsocket, &arch, sizeof(arch), 0);
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

	case CMD_STARTDEBUG:
	{
		printf("[NOT SUPPORT!] CMD_STARTDEBUG\n");
		break;
	}
	case CMD_WAITFORDEBUGEVENT:
	{
		printf("[NOT SUPPORT!] CMD_WAITFORDEBUGEVENT\n");
		break;
	}
	case CMD_CONTINUEFROMDEBUGEVENT:
	{
		printf("[NOT SUPPORT!] CMD_CONTINUEFROMDEBUGEVENT\n");
		break;
	}
	case CMD_SETBREAKPOINT:
	{
		printf("[NOT SUPPORT!] CMD_SETBREAKPOINT\n");
		break;
	}
	case CMD_REMOVEBREAKPOINT:
	{
		printf("[NOT SUPPORT!] CMD_REMOVEBREAKPOINT\n");
		break;
	}

	case CMD_GETTHREADCONTEXT:
	{
		printf("[NOT SUPPORT!] CMD_GETTHREADCONTEXT\n");
		break;
	}
	case CMD_SETTHREADCONTEXT:
	{
		printf("[NOT SUPPORT!] CMD_SETTHREADCONTEXT\n");
		break;
	}
	case CMD_SUSPENDTHREAD:
	{
		printf("[NOT SUPPORT!] CMD_SUSPENDTHREAD\n");
		break;
	}
	case CMD_RESUMETHREAD:
	{
		printf("[NOT SUPPORT!] CMD_RESUMETHREAD\n");
		break;
	}
	case CMD_CLOSEHANDLE:
	{
		HANDLE h;
		if (recvall(currentsocket, &h, sizeof(h), MSG_WAITALL) > 0) {
			printf("CMD_CLOSEHANDLE(%d)\n", h);
			CApi::CloseHandle(h);
			int r = 1;
			sendall(currentsocket, &r, sizeof(r), 0); //stupid naggle
		} else {
			printf("Error during read for CMD_CLOSEHANDLE\n");
			close(currentsocket);
			fflush(stdout);
			return 0;
		}
		break;
	}

	case CMD_CREATETOOLHELP32SNAPSHOT:
	{
		CeCreateToolhelp32Snapshot params;
		HANDLE result;

		printf("CMD_CREATETOOLHELP32SNAPSHOT\n");

		if (recvall(currentsocket, &params, sizeof(CeCreateToolhelp32Snapshot), MSG_WAITALL) > 0) {
			//printf("Calling CreateToolhelp32Snapshot\n");

			result = CApi::CreateToolhelp32Snapshot(params.dwFlags, params.th32ProcessID);

			printf("result of CreateToolhelp32Snapshot=%d\n", result);

			fflush(stdout);
			sendall(currentsocket, &result, sizeof(HANDLE), 0);

		} else {
			printf("Error during read for CMD_CREATETOOLHELP32SNAPSHOT\n");
			fflush(stdout);
			close(currentsocket);
			return 0;
		}

		break;
	}

	case CMD_PROCESS32FIRST: //obsolete
	case CMD_PROCESS32NEXT:
	{
		HANDLE toolhelpsnapshot;
		if (recvall(currentsocket, &toolhelpsnapshot, sizeof(toolhelpsnapshot), MSG_WAITALL) > 0) {
			//printf("CMD_PROCESS32FIRST || CMD_PROCESS32NEXT %d\n", toolhelpsnapshot);

			ProcessListEntry pe;
			BOOL result = FALSE;
			CeProcessEntry *r;
			int size;

			if (command == CMD_PROCESS32FIRST) {
				result = CApi::Process32First(toolhelpsnapshot, pe);
				//printf("Process32First result=%d\n", result);
			} else {
				result = CApi::Process32Next(toolhelpsnapshot, pe);
				//printf("Process32Next result=%d\n", result);
			}

			if (result) {
				size = sizeof(CeProcessEntry) + pe.ProcessName.length();
				r = (CeProcessEntry*)malloc(size);
				r->processnamesize = pe.ProcessName.length();
				r->pid = pe.PID;
				memcpy((char *)r + sizeof(CeProcessEntry), pe.ProcessName.c_str(), r->processnamesize);
			} else {
				size = sizeof(CeProcessEntry);
				r = (CeProcessEntry*)malloc(size);
				r->processnamesize = 0;
				r->pid = 0;
			}
			r->result = result;
			sendall(currentsocket, r, size, 0);
			free(r);

		}
		break;
	}

	case CMD_MODULE32FIRST: //slightly obsolete now
	case CMD_MODULE32NEXT:
	{
		HANDLE toolhelpsnapshot;
		if (recvall(currentsocket, &toolhelpsnapshot, sizeof(toolhelpsnapshot), MSG_WAITALL) > 0) {
			BOOL result;
			ModuleListEntry me;
			CeModuleEntry *r;
			int size;

			if (command == CMD_MODULE32FIRST) {
				result = CApi::Module32First(toolhelpsnapshot, me);
			} else {
				result = CApi::Module32Next(toolhelpsnapshot, me);
			}

			if (result) {
				size = sizeof(CeModuleEntry) + me.moduleName.length();
				r = (CeModuleEntry*)malloc(size);
				r->modulebase = me.baseAddress;
				r->modulesize = me.moduleSize;
				r->modulenamesize = me.moduleName.length();
				//printf("%s\n", me.moduleName.c_str());

				// Sending %s size %x\n, me.moduleName, r->modulesize
				memcpy((char *)r + sizeof(CeModuleEntry), me.moduleName.c_str(), r->modulenamesize);
			} else {
				size = sizeof(CeModuleEntry);
				r = (CeModuleEntry*)malloc(size);
				r->modulebase = 0;
				r->modulesize = 0;
				r->modulenamesize = 0;
			}

			r->result = result;

			sendall(currentsocket, r, size, 0);

			free(r);
		}
		break;
	}

	case CMD_OPENPROCESS:
	{
		int pid = 0;

		r = recvall(currentsocket, &pid, sizeof(int), MSG_WAITALL);
		if (r > 0) {
			int processhandle;

			printf("OpenProcess(%d)\n", pid);
			processhandle = CApi::OpenProcess(pid);

			printf("processhandle=%d\n", processhandle);
			sendall(currentsocket, &processhandle, sizeof(int), 0);
		} else {
			printf("Error\n");
			fflush(stdout);
			close(currentsocket);
			return 0;
		}
		break;
	}

	case CMD_READPROCESSMEMORY:
	{
		CeReadProcessMemoryInput c;

		r = recvall(currentsocket, &c, sizeof(c), MSG_WAITALL);
		if (r > 0) {
			CeReadProcessMemoryOutput *o = NULL;
			o = (CeReadProcessMemoryOutput*)malloc(sizeof(CeReadProcessMemoryOutput) + c.size);
			memset(o, 0, sizeof(CeReadProcessMemoryOutput) + c.size);

			o->read = CApi::ReadProcessMemory(c.handle, (void *)(uintptr_t)c.address, &o[1], c.size);

			if (c.compress) {
				//printf("ReadProcessMemory compress %d %p %d\n", c.handle, c.address, c.size);

				//compress the output
#define COMPRESS_BLOCKSIZE (64*1024)
				int i;
				unsigned char *uncompressed = (unsigned char *)&o[1];
				uint32_t uncompressedSize = o->read;
				uint32_t compressedSize = 0;
				int maxBlocks = 1 + (c.size / COMPRESS_BLOCKSIZE);

				unsigned char **compressedBlocks = (unsigned char**)malloc(maxBlocks * sizeof(unsigned char *)); //send in blocks of 64kb and reallocate the pointerblock if there's not enough space
				int currentBlock = 0;

				z_stream strm;
				strm.zalloc = Z_NULL;
				strm.zfree = Z_NULL;
				strm.opaque = Z_NULL;
				deflateInit(&strm, c.compress);

				compressedBlocks[currentBlock] = (unsigned char*)malloc(COMPRESS_BLOCKSIZE);
				strm.avail_out = COMPRESS_BLOCKSIZE;
				strm.next_out = compressedBlocks[currentBlock];

				strm.next_in = uncompressed;
				strm.avail_in = uncompressedSize;

				while (strm.avail_in) {
					r = deflate(&strm, Z_NO_FLUSH);
					if (r != Z_OK) {
						if (r == Z_STREAM_END)
							break;
						else {
							printf("Error while compressing\n");
							break;
						}
					}

					if (strm.avail_out == 0) {
						//new output block
						currentBlock++;
						if (currentBlock >= maxBlocks) {
							//list was too short, reallocate
							printf("Need to realloc the pointerlist (p1)\n");

							maxBlocks *= 2;
							compressedBlocks = (unsigned char**)realloc(compressedBlocks, maxBlocks * sizeof(unsigned char*));
						}
						compressedBlocks[currentBlock] = (unsigned char*)malloc(COMPRESS_BLOCKSIZE);
						strm.avail_out = COMPRESS_BLOCKSIZE;
						strm.next_out = compressedBlocks[currentBlock];
					}
				}
				// finishing compressiong
				while (1) {

					r = deflate(&strm, Z_FINISH);

					if (r == Z_STREAM_END)
						break; //done

					if (r != Z_OK) {
						printf("Failure while finishing compression:%d\n", r);
						break;
					}

					if (strm.avail_out == 0) {
						//new output block
						currentBlock++;
						if (currentBlock >= maxBlocks) {
							//list was too short, reallocate
							printf("Need to realloc the pointerlist (p2)\n");
							maxBlocks *= 2;
							compressedBlocks = (unsigned char**)realloc(compressedBlocks, maxBlocks * sizeof(unsigned char*));
						}
						compressedBlocks[currentBlock] = (unsigned char*)malloc(COMPRESS_BLOCKSIZE);
						strm.avail_out = COMPRESS_BLOCKSIZE;
						strm.next_out = compressedBlocks[currentBlock];
					}
				}
				deflateEnd(&strm);

				compressedSize = strm.total_out;
				// Sending compressed data
				sendall(currentsocket, &uncompressedSize, sizeof(uncompressedSize), MSG_MORE); //followed by the compressed size
				sendall(currentsocket, &compressedSize, sizeof(compressedSize), MSG_MORE); //the compressed data follows
				for (i = 0; i <= currentBlock; i++) {
					if (i != currentBlock)
						sendall(currentsocket, compressedBlocks[i], COMPRESS_BLOCKSIZE, MSG_MORE);
					else
						sendall(currentsocket, compressedBlocks[i], COMPRESS_BLOCKSIZE - strm.avail_out, 0); //last one, flush

					free(compressedBlocks[i]);
				}
				free(compressedBlocks);
			} else {
				//printf("ReadProcessMemory Uncompress %d %p %d\n", c.handle, c.address, c.size);
				sendall(currentsocket, o, sizeof(CeReadProcessMemoryOutput) + o->read, 0);
			}

			if (o) {
				free(o);
			}
		}
		break;
	}

	case CMD_WRITEPROCESSMEMORY:
	{
		CeWriteProcessMemoryInput c;

		printf("CMD_WRITEPROCESSMEMORY:\n");

		r = recvall(currentsocket, &c, sizeof(c), MSG_WAITALL);
		if (r > 0) {
			CeWriteProcessMemoryOutput o;
			unsigned char *buf;

			printf("recv returned %d bytes\n", r);
			printf("c.size=%d\n", c.size);

			if (c.size) {
				buf = (unsigned char *)malloc(c.size);

				r = recvall(currentsocket, buf, c.size, MSG_WAITALL);
				if (r > 0) {
					printf("received %d bytes for the buffer. Wanted %d\n", r, c.size);
					o.written = CApi::WriteProcessMemory(c.handle, (void *)(uintptr_t)c.address, buf, c.size);

					r = sendall(currentsocket, &o, sizeof(CeWriteProcessMemoryOutput), 0);
					printf("wpm: returned %d bytes to caller\n", r);

				} else {
					printf("wpm recv error while reading the data\n");
				}
				free(buf);
			} else {
				printf("wpm with a size of 0 bytes");
				o.written = 0;
				r = sendall(currentsocket, &o, sizeof(CeWriteProcessMemoryOutput), 0);
				printf("wpm: returned %d bytes to caller\n", r);
			}
		} else {
			printf("RPM: recv failed\n");
		}
		break;
	}
	case CMD_VIRTUALQUERYEXFULL:
	{
		//printf("CMD_VIRTUALQUERYEXFULL\n");

		CeVirtualQueryExFullInput c;
		CeVirtualQueryExFullOutput o;

		r = recvall(currentsocket, &c, sizeof(c), MSG_WAITALL);
		if (r > 0) {
			std::vector<RegionInfo> vRinfo;
			if (CApi::VirtualQueryExFull(c.handle, c.flags, vRinfo)) {
				//printf("VirtualQueryExFull ok %d\n", vRinfo.size());
				uint32_t count = vRinfo.size();
				sendall(currentsocket, &count, sizeof(count), 0);

				for (RegionInfo r : vRinfo) {
					sendall(currentsocket, &r, sizeof(r), 0);
				}
				//printf("VirtualQueryExFull all %d\n", vRinfo.size());
			} else {
				//printf("VirtualQueryExFull no %d\n", vRinfo.size());
				uint32_t count = 0;
				sendall(currentsocket, &count, sizeof(count), 0);
			}
		}
		break;
	}
	case CMD_GETREGIONINFO:
	case CMD_VIRTUALQUERYEX:
	{

		CeVirtualQueryExInput c;
		r = recvall(currentsocket, &c, sizeof(c), MSG_WAITALL);
		if (r > 0) {
			RegionInfo rinfo;
			CeVirtualQueryExOutput o;
			if (sizeof(uintptr_t) == 4) {
				if (c.baseaddress > 0xFFFFFFFF) {
					o.result = 0;
					sendall(currentsocket, &o, sizeof(o), 0);
					break;
				}
			}

			std::string mapsline;

			if (command == CMD_VIRTUALQUERYEX) {
				//printf("CMD_VIRTUALQUERYEX\n");
				o.result = CApi::VirtualQueryEx(c.handle, c.baseaddress, rinfo, mapsline);
			} else if (command == CMD_GETREGIONINFO) {
				//printf("CMD_GETREGIONINFO\n");
				o.result = CApi::VirtualQueryEx(c.handle, c.baseaddress, rinfo, mapsline);
			}

			o.protection = rinfo.protection;
			o.baseaddress = rinfo.baseaddress;
			o.type = rinfo.type;
			o.size = rinfo.size;

			if (command == CMD_VIRTUALQUERYEX) {
				sendall(currentsocket, &o, sizeof(o), 0);
			} else	if (command == CMD_GETREGIONINFO) {
				sendall(currentsocket, &o, sizeof(o), MSG_MORE);
				{
					uint8_t size = mapsline.length();
					sendall(currentsocket, &size, sizeof(size), MSG_MORE);

					if (size) {
						sendall(currentsocket, (void*)mapsline.c_str(), size, 0);
					}

				}
			}


		}
		break;
	}


	case CMD_GETSYMBOLLISTFROMFILE:
	{
		//get the list and send it to the client
		//zip it first
		uint32_t symbolpathsize;

		printf("CMD_GETSYMBOLLISTFROMFILE\n");

		if (recvall(currentsocket, &symbolpathsize, sizeof(symbolpathsize), MSG_WAITALL) > 0) {
			char *symbolpath = (char *)malloc(symbolpathsize + 1);
			symbolpath[symbolpathsize] = '\0';

			if (recvall(currentsocket, symbolpath, symbolpathsize, MSG_WAITALL) > 0) {
				unsigned char *output = NULL;

				printf("symbolpath=%s\n", symbolpath);

				if (memcmp("/dev/", symbolpath, 5) != 0) //don't even bother if it's a /dev/ file
				{
					//因为这里需要open本地的.so文件，有可能会被反调试手段侦测到，而且作用不太大，所以暂时屏蔽掉。
					//解决方法一（待实现）：在被调试进程启动前，读取本地.so文件
					//GetSymbolListFromFile(symbolpath, &output);
				}
				if (output) {
					printf("output is not NULL (%p)\n", output);

					fflush(stdout);

					printf("Sending %d bytes\n", *(uint32_t *)&output[4]);
					sendall(currentsocket, output, *(uint32_t *)&output[4], 0); //the output buffer contains the size itself
					free(output);
				} else {

					printf("Sending 8 bytes (fail)\n");
					uint64_t fail = 0;
					sendall(currentsocket, &fail, sizeof(fail), 0); //just write 0
				}
			} else {
				printf("Failure getting symbol path\n");
				close(currentsocket);
			}

			free(symbolpath);
		}
		break;
	}

	case CMD_LOADEXTENSION:
	{
		printf("[NOT SUPPORT!] CMD_LOADEXTENSION\n");
		break;
	}

	case CMD_ALLOC:
	{
		printf("[NOT SUPPORT!] CMD_ALLOC\n");
		break;
	}
	case CMD_FREE:
	{
		printf("[NOT SUPPORT!] CMD_FREE\n");
		break;
	}

	case CMD_CREATETHREAD:
	{
		printf("[NOT SUPPORT!] CMD_CREATETHREAD\n");
		break;
	}
	case CMD_LOADMODULE:
	{
		printf("[NOT SUPPORT!] CMD_LOADMODULE\n");
		break;
	}
	case CMD_SPEEDHACK_SETSPEED:
	{
		printf("[NOT SUPPORT!] CMD_SPEEDHACK_SETSPEED\n");
		break;
	}
	case CMD_AOBSCAN:
	{
		CeAobScanInput c;
		printf("CESERVER: CMD_AOBSCAN\n");
		if (recvall(currentsocket, &c, sizeof(c), 0) > 0) {

			int n = c.scansize;
			char* data = (char*)malloc(n * 2);
			uint64_t* match_addr = (uint64_t*)malloc(sizeof(uint64_t) * MAX_HIT_COUNT);

			if (recvall(currentsocket, data, n * 2, 0) > 0) {
				char* pattern = (char*)malloc(n);
				char* mask = (char*)malloc(n);

				memcpy(pattern, data, n);
				memcpy(mask, &data[n], n);
				int ret = 0; //AOBScan(c.hProcess, pattern, mask, c.start, c.end, c.inc, c.protection, match_addr);
				printf("HIT_COUNT:%d\n", ret);
				free(pattern);
				free(mask);
				sendall(currentsocket, &ret, 4, 0);
				sendall(currentsocket, match_addr, sizeof(uint64_t)* ret, 0);
			}
			free(data);
			free(match_addr);
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
	addr.sin_port = htons(3296);
	i = bind(s, (struct sockaddr *)&addr, sizeof(addr));

	if (i >= 0) {
		while (1) {
			memset(&addr_client, 0, sizeof(addr_client));
			addr_client.sin_family = PF_INET;
			addr_client.sin_addr.s_addr = INADDR_ANY;
			addr_client.sin_port = htons(3296);

			clisize = sizeof(addr_client);

			i = recvfrom(s, &packet, sizeof(packet), 0, (struct sockaddr *)&addr_client, &clisize);

			//i=recv(s, &v, sizeof(v), 0);
			if (i >= 0) {

				printf("Identifier thread received a message :%d\n", v);
				printf("sizeof(packet)=%d\n", (int)sizeof(packet));

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
	//初始化读写驱动
	if (!CApi::InitReadWriteDriver(argc > 1 ? argv[1] : NULL)) {
		printf("Init read write driver failed.\n");
		return 0;
	}


	socklen_t clisize;
	struct sockaddr_in addr, addr_client;

	printf("listening on port %d\n", PORT);

	printf("CEServer. Waiting for client connection\n");

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