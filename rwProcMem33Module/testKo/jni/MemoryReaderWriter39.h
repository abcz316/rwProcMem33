#ifndef MEMORY_READER_WRITER_H_
#define MEMORY_READER_WRITER_H_

#include <vector>
#include <mutex>

#include "IMemReaderWriterProxy.h"
#ifdef __linux__
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <elf.h>
#include <errno.h>
#include <malloc.h>
#include <random>
#include <algorithm>
#include <filesystem>
#include "IoctlBufferPool.h"
//安静输出模式
#define QUIET_PRINTF
#endif /*__linux__*/

class CMemoryReaderWriter : public IMemReaderWriterProxy {
public:
	CMemoryReaderWriter() {}
	~CMemoryReaderWriter() { DisconnectDriver(); }

	// 连接驱动
	// 参数 procNodeAuthKey：隐蔽节点连接密钥
	// 返回值：>=0成功，<0错误码
	int ConnectDriver(const std::string& procNodeAuthKey) {
		return _InternalConnectDriver(procNodeAuthKey);
	}

	// 断开驱动
	// 返回值：TRUE成功，FALSE失败
	BOOL DisconnectDriver() {
		return _InternalDisconnectDriver();
	}

	// 检查驱动连接状态
	// 返回值：TRUE已连接，FALSE未连接
	BOOL IsDriverConnected() {
		return _InternalIsDriverConnected();
	}

	// 驱动_打开进程
	// 参数 pid：目标进程PID
	// 返回值：成功返回进程句柄，失败返回0
	uint64_t OpenProcess(uint64_t pid) {
		return _InternalOpenProcess(pid);
	}

	// 驱动_读取进程内存
	// 参数 hProcess：				进程句柄
	// 参数 lpBaseAddress：			进程内存地址
	// 参数 lpBuffer：				待接收数据的缓冲区
	// 参数 nSize：					待接收数据的缓冲区大小
	// 参数 lpNumberOfBytesRead		返回实际读取的字节数
	// 参数 bIsForceRead：			读取模式，FALSE-安全读取；TRUE-暴力读取
	// 返回值：TRUE成功，FALSE失败
	BOOL ReadProcessMemory(uint64_t hProcess,
						uint64_t lpBaseAddress,
						void* lpBuffer,
						size_t nSize,
						size_t* lpNumberOfBytesRead = NULL,
						BOOL bIsForceRead = FALSE) override {
		return _InternalReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead, bIsForceRead);
	}

	// 驱动_写入进程内存
	// 参数 hProcess：				进程句柄
	// 参数 lpBaseAddress：			进程内存地址
	// 参数 lpBuffer：				欲写入数据的缓冲区
	// 参数 nSize：					欲写入数据的缓冲区大小
	// 参数 lpNumberOfBytesWritten	返回实际写入的字节数
	// 参数 bIsForceWrite：			写入模式，FALSE-安全写入；TRUE-暴力写入
	// 返回值：TRUE成功，FALSE失败
	BOOL WriteProcessMemory(uint64_t hProcess,
							uint64_t lpBaseAddress,
							void* lpBuffer,
							size_t nSize,
							size_t* lpNumberOfBytesWritten = NULL,
							BOOL bIsForceWrite = FALSE) override {
		return _InternalWriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten, bIsForceWrite);
	}

	// 驱动_关闭进程句柄
	// 参数 hProcess：进程句柄
	// 返回值：TRUE成功，FALSE失败
	BOOL CloseHandle(uint64_t hProcess) {
		return _InternalCloseHandle(hProcess);
	}

	// 驱动_获取进程内存块列表
	// 参数 hProcess：	进程句柄
	// 参数 showPhy：	FALSE-显示全部内存；TRUE-仅显示物理页；注非物理页驱动不可读
	// 参数 vOutput：	输出内存块列表结果
	// 返回值：TRUE成功，FALSE失败
	BOOL VirtualQueryExFull(uint64_t hProcess, BOOL showPhy, std::vector<DRIVER_REGION_INFO>& vOutput) override {
		return _InternalVirtualQueryExFull(hProcess, showPhy, vOutput);
	}

	// 驱动_检查进程内存地址有效性
	// 参数 hProcess：		进程句柄
	// 参数 lpBaseAddress：进程内存地址
	// 返回值：TRUE有效，FALSE无效
	BOOL CheckProcessMemAddrValid(uint64_t hProcess, uint64_t lpBaseAddress) override {
		return _InternalCheckProcessMemAddrValid(hProcess, lpBaseAddress);
	}

	// 驱动_获取PID列表
	// 参数 vOutput：输出进程PID列表
	// 返回值：TRUE成功，FALSE失败
	BOOL GetPidList(std::vector<int>& vOutput) {
		return _InternalGetPidList(vOutput);
	}

	// 驱动_提升进程权限至Root
	// 参数 hProcess：进程句柄
	// 返回值：TRUE成功，FALSE失败
	BOOL SetProcessRoot(uint64_t hProcess) {
		return _InternalSetProcessRoot(hProcess);
	}

	// 驱动_获取进程物理内存占用大小
	// 参数 hProcess：进程句柄
	// 参数 outRss：输出占用字节数
	// 返回值：TRUE成功，FALSE失败
	BOOL GetProcessPhyMemSize(uint64_t hProcess, uint64_t& outRss) {
		return _InternalGetProcessPhyMemSize(hProcess, outRss);
	}

	// 驱动_获取进程命令行
	// 参数 hProcess：进程句柄
	// 参数 lpOutCmdlineBuf：待接收数据的缓冲区；
	// 参数 bufSize：待接收数据的缓冲区大小
	// 返回值：TRUE成功，FALSE失败
	BOOL GetProcessCmdline(uint64_t hProcess, char* lpOutCmdlineBuf, size_t bufSize) {
		return _InternalGetProcessCmdline(hProcess, lpOutCmdlineBuf, bufSize);
	}

	// 驱动_隐藏内核模块
	// 返回值：TRUE成功，FALSE失败
	BOOL HideKernelModule() {
		return _InternalHideKernelModule();
	}

	// 驱动_获取连接FD
	// 返回值：文件描述符
	int GetLinkFD() {
		return _InternalGetLinkFD();
	}

	// 驱动_设置连接FD
	// 参数 fd：文件描述符
	void SetLinkFD(int fd) {
		_InternalSetLinkFD(fd);
	}

private:
	int _InternalConnectDriver(const std::string& procNodeAuthKey) {
#ifdef __linux__
		if (m_nFd >= 0) { return TRUE; }
		m_nFd = _rwProcMemDriver_Connect(procNodeAuthKey);
		if (m_nFd < 0) {
			return m_nFd;
		}
		_rwProcMemDriver_InitDeviceInfo(m_nFd);
		return 0;
#else 
		return -1;
#endif
	}

	BOOL _InternalDisconnectDriver() {
#ifdef __linux__
		if (m_nFd >= 0) {
			_rwProcMemDriver_Disconnect(m_nFd);
			m_nFd = -1;
			return TRUE;
		}
#endif
		return FALSE;
	}

	BOOL _InternalIsDriverConnected() {
#ifdef __linux__
		return m_nFd >= 0 ? TRUE : FALSE;
#else
		return FALSE;
#endif
	}

	BOOL _InternalHideKernelModule() {
#ifdef __linux__
		return _rwProcMemDriver_HideKernelModule(m_nFd);
#else
		return FALSE;
#endif
	}

	uint64_t _InternalOpenProcess(uint64_t pid) {
#ifdef __linux__
		return _rwProcMemDriver_OpenProcess(m_nFd, pid);
#else
		return FALSE;
#endif
	}

	BOOL _InternalReadProcessMemory(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void* lpBuffer,
		size_t nSize,
		size_t* lpNumberOfBytesRead = NULL,
		BOOL bIsForceRead = FALSE) {
#ifdef __linux__
		return _rwProcMemDriver_ReadProcessMemory(
			m_nFd,
			hProcess,
			lpBaseAddress,
			lpBuffer,
			nSize,
			lpNumberOfBytesRead,
			bIsForceRead);
#else
		return FALSE;
#endif
	}

	BOOL _InternalWriteProcessMemory(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void* lpBuffer,
		size_t nSize,
		size_t* lpNumberOfBytesWritten = NULL,
		BOOL bIsForceWrite = FALSE) {
#ifdef __linux__
		return _rwProcMemDriver_WriteProcessMemory(
			m_nFd,
			hProcess,
			lpBaseAddress,
			lpBuffer,
			nSize,
			lpNumberOfBytesWritten,
			bIsForceWrite);
#else
		return FALSE;
#endif
	}

	BOOL _InternalCloseHandle(uint64_t hProcess) {
#ifdef __linux__
		return _rwProcMemDriver_CloseHandle(m_nFd, hProcess);
#else
		return FALSE;
#endif
	}

	BOOL _InternalVirtualQueryExFull(uint64_t hProcess, BOOL showPhy, std::vector<DRIVER_REGION_INFO>& vOutput) {
#ifdef __linux__
		return _rwProcMemDriver_VirtualQueryExFull(m_nFd, hProcess, showPhy, vOutput);
#else
		return FALSE;
#endif
	}

	BOOL _InternalCheckProcessMemAddrValid(uint64_t hProcess, uint64_t lpBaseAddress) {
#ifdef __linux__
		return _rwProcMemDriver_CheckProcessMemAddrValid(m_nFd, hProcess, lpBaseAddress);
#else
		return FALSE;
#endif
	}

	BOOL _InternalGetPidList(std::vector<int>& vOutput) {
#ifdef __linux__
		return _rwProcMemDriver_GetPidList(m_nFd, vOutput);
#else
		return FALSE;
#endif
	}

	BOOL _InternalSetProcessRoot(uint64_t hProcess) {
#ifdef __linux__
		return _rwProcMemDriver_SetProcessRoot(m_nFd, hProcess);
#else
		return FALSE;
#endif
	}

	BOOL _InternalGetProcessPhyMemSize(uint64_t hProcess, uint64_t& outRss) {
#ifdef __linux__
		return _rwProcMemDriver_GetProcessPhyMemSize(m_nFd, hProcess, &outRss);
#else
		return FALSE;
#endif
	}

	BOOL _InternalGetProcessCmdline(uint64_t hProcess, char* lpOutCmdlineBuf, size_t bufSize) {
#ifdef __linux__
		return _rwProcMemDriver_GetProcessCmdline(m_nFd, hProcess, lpOutCmdlineBuf, bufSize);
#else
		return FALSE;
#endif
	}

	int _InternalGetLinkFD() {
#ifdef __linux__
		return m_nFd;
#else
		return -1;
#endif
	}

	void _InternalSetLinkFD(int fd) {
#ifdef __linux__
		m_nFd = fd;
#endif
	}

#ifdef __linux__
	#ifdef QUIET_PRINTF
	#undef TRACE
	#define TRACE(fmt, ...)
	#else
	#define TRACE(fmt, ...) printf(fmt, ##__VA_ARGS__)
	#endif
	#define MY_PATH_MAX_LEN 1024
	#define MY_TASK_COMM_LEN 16 
	enum {
		CMD_INIT_DEVICE_INFO = 1, 		// 初始化设备信息
		CMD_OPEN_PROCESS, 				// 打开进程
		CMD_READ_PROCESS_MEMORY,       	// 读取进程内存
		CMD_WRITE_PROCESS_MEMORY,      	// 写入进程内存
		CMD_CLOSE_PROCESS, 				// 关闭进程
		CMD_GET_PROCESS_MAPS_COUNT, 	// 获取进程的内存块地址数量
		CMD_GET_PROCESS_MAPS_LIST, 		// 获取进程的内存块地址列表
		CMD_CHECK_PROCESS_ADDR_PHY,		// 检查进程内存是否有物理内存位置
		CMD_GET_PID_LIST,				// 获取进程PID列表
		CMD_SET_PROCESS_ROOT,			// 提升进程权限到Root
		CMD_GET_PROCESS_RSS,			// 获取进程的物理内存占用大小
		CMD_GET_PROCESS_CMDLINE_ADDR,	// 获取进程cmdline的内存地址
		CMD_HIDE_KERNEL_MODULE,			// 隐藏驱动
	};

	#pragma pack(push,1)
	struct IoctlRequest {
		char     cmd = 0;        // 操作命令
		uint64_t param1 = 0;     // 参数1
		uint64_t param2 = 0;     // 参数2
		uint64_t param3 = 0;     // 参数3
		uint64_t bufSize = 0;    // 紧随其后的动态数据长度
	};
	struct init_device_info {
		int pid = 0;
		int tid = 0;
		char myName[MY_TASK_COMM_LEN + 1] = {0};
		char myAuxv[1024] = {0};
		int myAuxvSize = 0;
	};
	struct map_entry {
		uint64_t      start = 0;
		uint64_t      end = 0;
		unsigned char flags[4] = {0};
		char          path[MY_PATH_MAX_LEN] = {0};
	};
	struct arg_info {
		uint64_t arg_start = 0;
		uint64_t arg_end = 0;
	};
	#pragma pack(pop)

	inline ssize_t _rwProcMemDriver_MyIoctl(
		int      fd,
		char     cmd,
		uint64_t param1,
		uint64_t param2,
		uint64_t param3,
		char* buf,
		uint64_t   bufSize)
	{
		constexpr size_t headerSize = sizeof(IoctlRequest);
		size_t totalSize = headerSize + bufSize;

		static thread_local IoctlBufferPool pool;
		char* pBuf = pool.getBuffer(totalSize);
		if (!pBuf) return -ENOMEM;  

		IoctlRequest* req = reinterpret_cast<IoctlRequest*>(pBuf);
		req->cmd     = cmd;
		req->param1  = param1;
		req->param2  = param2;
		req->param3  = param3;
		req->bufSize = bufSize;
		if (bufSize > 0) {
			std::memcpy(pBuf + headerSize, buf, bufSize);
		}
		auto outRead = ::read(fd, pBuf, totalSize);
		if (bufSize > 0) {
			std::memcpy(buf, pBuf + headerSize, bufSize);
		}
		return outRead;
	}

	int _rwProcMemDriver_Connect(const std::string& procNodeAuthKey) {
	    namespace fs = std::filesystem;
		fs::path nodePath = fs::path("/proc") / procNodeAuthKey / procNodeAuthKey;
		const char* pathCStr = nodePath.c_str();
		if (chmod(pathCStr, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) != 0) {
			int err = errno;
			return -err;
		}
		int fd = open(pathCStr, O_RDWR);
		if (fd < 0) {
			int err = errno;
			return -err;
		}
		return fd;
	}

	BOOL _rwProcMemDriver_Disconnect(int nFd) {
		if (nFd < 0) { return FALSE; }
		close(nFd);
		return TRUE;
	}

	std::vector<uint8_t> generate_unique_non_zero_bytes(std::size_t count) {
		if (count == 0 || count > 255) {
			return {};
		}
		std::vector<uint8_t> pool(255);
		std::iota(pool.begin(), pool.end(), 1);
		std::random_device rd;
		std::mt19937 gen(rd());
		std::shuffle(pool.begin(), pool.end(), gen);
		return std::vector<uint8_t>(pool.begin(), pool.begin() + count);
	}

	BOOL _rwProcMemDriver_InitDeviceInfo(int nFd) {
		init_device_info oDevInfo = { 0 };
		oDevInfo.pid = getpid();
		oDevInfo.tid = gettid();
		std::vector<unsigned long> myAuxv = _GetAuxvSignature();
		int myAuxvByteCount = myAuxv.size() * sizeof(unsigned long);
		if(myAuxvByteCount == 0 || myAuxvByteCount >= sizeof(oDevInfo.myAuxv)) {
			return FALSE;
		}
		memcpy(&oDevInfo.myAuxv, reinterpret_cast<const uint8_t*>(myAuxv.data()), myAuxvByteCount);
		oDevInfo.myAuxvSize = myAuxvByteCount;
		char old_name[MY_TASK_COMM_LEN + 1] = {0};
		if (prctl(PR_GET_NAME, old_name, 0, 0, 0)) {
			return FALSE;
		}
		std::vector<uint8_t> random_name = generate_unique_non_zero_bytes(MY_TASK_COMM_LEN - 1);
		random_name.push_back(0);
		if (prctl(PR_SET_NAME, random_name.data(), 0, 0, 0)) {
			return FALSE;
		}
		strncpy(oDevInfo.myName, (char*)random_name.data(),  sizeof(oDevInfo.myName)  - 1);
		ssize_t ret = _rwProcMemDriver_MyIoctl(nFd,
									CMD_INIT_DEVICE_INFO, 0, 0, 0,
									(char*)&oDevInfo,
									sizeof(oDevInfo));

		if (prctl(PR_SET_NAME, old_name, 0, 0, 0)) {
			return FALSE;
		}
		TRACE("InitDeviceInfo ioctl return=%zd\n", ret);
		return (ret == 0) ? TRUE : FALSE;
	}

	BOOL _rwProcMemDriver_HideKernelModule(int nFd) {
		if (nFd < 0) { return FALSE; }
		ssize_t res = _rwProcMemDriver_MyIoctl(nFd, CMD_HIDE_KERNEL_MODULE, 0, 0, 0, NULL, 0);
		if (res != 0) {
			TRACE("HideKernelModule ioctl():%s\n", strerror(errno));
			return FALSE;
		}
		return TRUE;
	}

	uint64_t _rwProcMemDriver_OpenProcess(int nFd, uint64_t pid) {
		if (nFd < 0 || pid == 0) { return 0; }
		uint64_t handle = 0;
		ssize_t res = _rwProcMemDriver_MyIoctl(nFd, CMD_OPEN_PROCESS, pid, 0, 0, (char*)&handle, sizeof(handle));
		if (res != 0) {
			TRACE("OpenProcess ioctl():%s\n", strerror(errno));
			return 0;
		}
		return handle;
	}

	BOOL _rwProcMemDriver_ReadProcessMemory(
		int nFd,
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void * lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesRead,
		BOOL bIsForceRead) {
		if (lpBaseAddress == 0 || nFd < 0 || hProcess == 0 || nSize == 0) {
			return FALSE;
		}
		static thread_local IoctlBufferPool pool;
		ssize_t outOfRead = _rwProcMemDriver_MyIoctl(nFd, CMD_READ_PROCESS_MEMORY,
			hProcess, lpBaseAddress, bIsForceRead ? 1 : 0, (char*)lpBuffer, nSize);
		if (outOfRead <= 0) {
			TRACE("ReadProcessMemory ioctl(): %s\n", strerror(errno));
			return FALSE;
		}
		if (lpNumberOfBytesRead) {
			*lpNumberOfBytesRead = outOfRead;
		}
		return TRUE;
	}

	BOOL _rwProcMemDriver_WriteProcessMemory(
		int nFd,
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void * lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesWritten,
		BOOL bIsForceWrite) {
		if (lpBaseAddress == 0 || nFd < 0 || !hProcess || nSize == 0) {
			return FALSE;
		}
		static thread_local IoctlBufferPool pool;
		ssize_t outOfWrite = _rwProcMemDriver_MyIoctl(nFd, CMD_WRITE_PROCESS_MEMORY,
			hProcess, lpBaseAddress, bIsForceWrite ? 1 : 0, (char*)lpBuffer, nSize);
		if (outOfWrite <= 0) {
			TRACE("ReadProcessMemory ioctl(): %s\n", strerror(errno));
			return FALSE;
		}
		if (lpNumberOfBytesWritten) {
			*lpNumberOfBytesWritten = outOfWrite;
		}
		return TRUE;
	}

	BOOL _rwProcMemDriver_CloseHandle(int nFd, uint64_t hProcess) {
		if (nFd < 0 || !hProcess) { return FALSE; }
		if (_rwProcMemDriver_MyIoctl(nFd, CMD_CLOSE_PROCESS, hProcess, 0, 0, NULL, 0) != 0) {
			TRACE("CloseHandle ioctl():%s\n", strerror(errno));
			return FALSE;
		}
		return TRUE;
	}


	BOOL _rwProcMemDriver_VirtualQueryExFull(int nFd, uint64_t hProcess, BOOL showPhy, std::vector<DRIVER_REGION_INFO>& vOutput) {
		if (nFd < 0 || !hProcess) { return FALSE; }
		ssize_t count = _rwProcMemDriver_MyIoctl(nFd, CMD_GET_PROCESS_MAPS_COUNT, hProcess, 0, 0, NULL, 0);
		TRACE("VirtualQueryExFull count %zd\n", count);
		if (count <= 0) {
			TRACE("VirtualQueryExFull ioctl():%s\n", strerror(errno));
			return FALSE;
		}

		static thread_local IoctlBufferPool pool;
		uint64_t big_buf_len = sizeof(map_entry) * (count + 50);
		char*   big_buf     = pool.getBuffer(big_buf_len);
		if (!big_buf) return FALSE;
		memset(big_buf, 0, big_buf_len);
		ssize_t res = _rwProcMemDriver_MyIoctl(nFd, CMD_GET_PROCESS_MAPS_LIST, hProcess, 0, 0, big_buf, big_buf_len);
		TRACE("VirtualQueryExFull res %zd\n", res);
		if (res <= 0) {
			TRACE("VirtualQueryExFull ioctl():%s\n", strerror(errno));
			return FALSE;
		}
		auto entries = reinterpret_cast<map_entry*>(big_buf);
		for (ssize_t i = 0; i < res; ++i) {
			const map_entry& e = entries[i];
			DRIVER_REGION_INFO rInfo = {0};
			rInfo.baseaddress = e.start;
			rInfo.size        = e.end - e.start;
			bool r = e.flags[0], w = e.flags[1], x = e.flags[2];
			if (x) {
				rInfo.protection = w ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
			} else {
				if (w)           rInfo.protection = PAGE_READWRITE;
				else if (r)      rInfo.protection = PAGE_READONLY;
				else             rInfo.protection = PAGE_NOACCESS;
			}
			// 解析 type
			rInfo.type = (e.flags[3] ? MEM_MAPPED : MEM_PRIVATE);
			// 复制名字
        	strncpy(rInfo.name, e.path, sizeof(rInfo.name)-1);
			if (showPhy) {
				DRIVER_REGION_INFO cur = rInfo;
				bool in_phy = false;
				for (uint64_t addr = e.start; addr < e.end; addr += getpagesize()) {
					if (_rwProcMemDriver_CheckProcessMemAddrValid(nFd, hProcess, addr)) {
						if (!in_phy) {
							in_phy = true;
							cur.baseaddress = addr;
						}
					} else if (in_phy) {
						in_phy = false;
						cur.size = addr - cur.baseaddress;
						vOutput.push_back(cur);
					}
				}
				if (in_phy) {
					cur.size = e.end - cur.baseaddress;
					vOutput.push_back(cur);
				}
			} else {
				vOutput.push_back(rInfo);
			}
		}
		return TRUE;
	}

	BOOL _rwProcMemDriver_CheckProcessMemAddrValid(int nFd, uint64_t hProcess, uint64_t lpBaseAddress) {
		if (nFd < 0 || !hProcess) { return FALSE; }
		if (_rwProcMemDriver_MyIoctl(nFd, CMD_CHECK_PROCESS_ADDR_PHY, hProcess, lpBaseAddress, 0, NULL, 0) == 1) {
			return TRUE;
		}
		return FALSE;
	}

	BOOL _rwProcMemDriver_GetPidList(int nFd, std::vector<int>& vOutput) {
		if (nFd < 0) return FALSE;

		ssize_t count1 = _rwProcMemDriver_MyIoctl(nFd, CMD_GET_PID_LIST, 0, 0, 0, NULL, 0);
		if (count1 <= 0) {
			return FALSE;
		}

		static thread_local IoctlBufferPool pool;
		uint64_t len  = (uint64_t)count1 * sizeof(int);
		char*    buf     = pool.getBuffer(len);
		if (!buf) {
			return FALSE;
		}
		memset(buf, 0, len);
		ssize_t count2 = _rwProcMemDriver_MyIoctl(nFd, CMD_GET_PID_LIST, 0, 0, 0, buf, len);
		if (count2 != count1) {
			return FALSE;
		}

		for (int i = 0; i < count1; i++) {
			int pid = *(int*)(buf + i * sizeof(int));
			vOutput.push_back(pid);
		}
		return TRUE;
	}

	BOOL _rwProcMemDriver_SetProcessRoot(int nFd, uint64_t hProcess) {
		if (nFd < 0 || !hProcess) { return FALSE; }
		ssize_t res = _rwProcMemDriver_MyIoctl(nFd, CMD_SET_PROCESS_ROOT, hProcess, 0, 0, NULL, 0);
		if (res != 0) {
			TRACE("SetProcessRoot ioctl():%s\n", strerror(errno));
			return FALSE;
		}
		return TRUE;
	}

	BOOL _rwProcMemDriver_GetProcessPhyMemSize(int nFd, uint64_t hProcess, uint64_t *outRss) {
		if (nFd < 0 || !hProcess) { return FALSE; }
		uint64_t out = 0;
		ssize_t res = _rwProcMemDriver_MyIoctl(nFd, CMD_GET_PROCESS_RSS, hProcess, 0, 0, (char*)&out, sizeof(out));
		if (res != 0) {
			TRACE("GetProcessPhyMemSize ioctl():%s\n", strerror(errno));
			return FALSE;
		}
		*outRss = out;
		return TRUE;
	}
	
	BOOL _rwProcMemDriver_GetProcessCmdline(int nFd, uint64_t hProcess, char *lpOutCmdlineBuf, size_t bufSize) {
		if (nFd < 0 || !hProcess || bufSize <= 0) { return FALSE; }
		arg_info aginfo = { 0 };
		ssize_t res = _rwProcMemDriver_MyIoctl(nFd, CMD_GET_PROCESS_CMDLINE_ADDR, hProcess, 0, 0, (char*)&aginfo, sizeof(aginfo));
		if (res != 0) {
			TRACE("GetProcessCmdline ioctl():%s\n", strerror(errno));
			return FALSE;
		}
		if (aginfo.arg_start == 0) {
			return FALSE;
		}
		uint64_t len = aginfo.arg_end - aginfo.arg_start;
		if (len == 0) {
			return FALSE;
		}
		if (bufSize < len) {
			len = bufSize;
		}
		memset(lpOutCmdlineBuf, 0, bufSize);
		return _rwProcMemDriver_ReadProcessMemory(nFd, hProcess, aginfo.arg_start, lpOutCmdlineBuf, len, NULL, FALSE);
	}
#endif /*__linux__*/
	std::vector<unsigned long> _GetAuxvSignature() {
		std::vector<unsigned long> sig;
		int fd = open("/proc/self/auxv", O_RDONLY);
		if (fd < 0) {
			return sig;
		}

		while (true) {
			unsigned long a_type, a_val;
			ssize_t nr;
			nr = read(fd, &a_type, sizeof(a_type));
			if (nr != sizeof(a_type)) {
				break;
			}
			nr = read(fd, &a_val, sizeof(a_val));
			if (nr != sizeof(a_val)) {
				break;
			}
			sig.push_back(a_type);
			sig.push_back(a_val);
			if (a_type == AT_NULL) {
				break;
			}
		}
		close(fd);
		return sig;
	}
private:
	int m_nFd = -1;
};

#endif /* MEMORY_READER_WRITER_H_ */
