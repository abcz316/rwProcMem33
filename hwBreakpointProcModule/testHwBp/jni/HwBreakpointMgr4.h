#ifndef HW_BREAKPOINT_MANAGER_H_
#define HW_BREAKPOINT_MANAGER_H_

#include <stdio.h>
#include <stdint.h>
#include <vector>
#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <malloc.h>
#include <random>
#include <algorithm>
#include <filesystem>
#include "IoctlBufferPool.h"
#include <linux\perf_event.h>

typedef int BOOL;
#define TRUE 1
#define FALSE 0

#endif

enum {
	HW_BREAKPOINT_LEN_1 = 1,
	HW_BREAKPOINT_LEN_2 = 2,
	HW_BREAKPOINT_LEN_4 = 4,
	HW_BREAKPOINT_LEN_8 = 8,
};

enum {
	HW_BREAKPOINT_EMPTY = 0,
	HW_BREAKPOINT_R = 1,
	HW_BREAKPOINT_W = 2,
	HW_BREAKPOINT_RW = HW_BREAKPOINT_R | HW_BREAKPOINT_W,
	HW_BREAKPOINT_X = 4,
	HW_BREAKPOINT_INVALID = HW_BREAKPOINT_RW | HW_BREAKPOINT_X,
};
#pragma pack(1)
struct my_user_pt_regs {
	uint64_t regs[31];
	uint64_t sp;
	uint64_t pc;
	uint64_t pstate;
	uint64_t orig_x0;
	uint64_t syscallno;
};

struct HW_HIT_ITEM {
	uint64_t task_id;
	uint64_t hit_addr;
	uint64_t hit_time;
	struct my_user_pt_regs regs_info;
};
#pragma pack()

#ifdef __linux__
class CHwBreakpointMgr {
public:

	CHwBreakpointMgr() {}
	~CHwBreakpointMgr() { DisconnectDriver(); }


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

	// 驱动_关闭进程句柄
	// 参数 hProcess：进程句柄
	// 返回值：TRUE成功，FALSE失败
	BOOL CloseHandle(uint64_t hProcess) {
		return _InternalCloseHandle(hProcess);
	}

	// 驱动_获取CPU硬件执行断点支持数量
	// 返回值：数量
	int GetNumBRPS() {
		return _InternalGetNumBRPS();
	}

	// 驱动_获取CPU硬件访问断点支持数量
	// 返回值：数量
	int GetNumWRPS() {
		return _InternalGetNumWRPS();
	}

	
	// 驱动_安装进程硬件断点
	// 参数 hProcess：进程句柄
	// 参数 lpBaseAddress：进程内存地址
	// 参数 hwbpLen：硬件断点长度
	// 参数 hwbpType：硬件断点类型
	// 返回值：成功返回硬件断点句柄，失败返回0
	uint64_t InstProcessHwBp(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		unsigned int hwbpLen,
		unsigned int hwbpType
	) {
		return _InternalInstProcessHwBp(hProcess, lpBaseAddress, hwbpLen, hwbpType);
	}


	// 驱动_卸载进程硬件断点
	// 参数 hHwbp：硬件断点句柄
	// 返回值：TRUE成功，FALSE失败
	BOOL UninstProcessHwBp(uint64_t hHwbp) {
		return _InternalUninstProcessHwBp(hHwbp);
	}

	// 驱动_暂停进程硬件断点
	// 参数 hHwbp：硬件断点句柄
	// 返回值：TRUE成功，FALSE失败
	BOOL SuspendProcessHwBp(uint64_t hHwbp) {
		return _InternalSuspendProcessHwBp(hHwbp);
	}
	
	// 驱动_恢复进程硬件断点
	// 参数 hHwbp：硬件断点句柄
	// 返回值：TRUE成功，FALSE失败
	BOOL ResumeProcessHwBp(uint64_t hHwbp) {
		return _InternalResumeProcessHwBp(hHwbp);
	}

	// 驱动_读取硬件断点命中记录信息
	// 参数 hHwbp：硬件断点句柄
	// 参数 nHitTotalCount：输出命中总数
	// 参数 vOutput：输出命中信息
	// 返回值：TRUE成功，FALSE失败
	BOOL ReadHwBpInfo(uint64_t hHwbp, uint64_t & nHitTotalCount, std::vector<HW_HIT_ITEM> & vOutput) {
		return _InternalReadHwBpInfo(hHwbp, nHitTotalCount, vOutput);
	}

	// 驱动_设置无条件Hook跳转
	// 参数 pc：硬件执行断点触发后，要跳转到的进程内存地址
	// 返回值：TRUE成功，FALSE失败
	BOOL SetHookPC(uint64_t pc) {
		return _hwbpProcDriver_SetHookPC(m_nFd, pc);
	}
	
	// 驱动_隐藏内核模块
	// 返回值：TRUE成功，FALSE失败
	BOOL HideKernelModule() {
		return _InternalHideKernelModule();
	}

private:
	int _InternalConnectDriver(const std::string& procNodeAuthKey) {
		if (m_nFd >= 0) { return TRUE; }
		m_nFd = _hwbpProcDriver_Connect(procNodeAuthKey);
		if (m_nFd < 0) {
			return m_nFd;
		}
		return 0;
	}

	BOOL _InternalDisconnectDriver() {
		if (m_nFd >= 0) {
			_hwbpProcDriver_Disconnect(m_nFd);
			m_nFd = -1;
			return TRUE;
		}
		return FALSE;
	}

	BOOL _InternalIsDriverConnected() {
		return m_nFd >= 0 ? TRUE : FALSE;
	}

	BOOL _InternalHideKernelModule() {
		return _hwbpProcDriver_HideKernelModule(m_nFd);
	}

	uint64_t _InternalOpenProcess(uint64_t pid) {
		return _hwbpProcDriver_OpenProcess(m_nFd, pid);
	}

	BOOL _InternalCloseHandle(uint64_t hProcess) {
		return _hwbpProcDriver_CloseHandle(m_nFd, hProcess);
	}

	int _InternalGetNumBRPS() {
		return _hwbpProcDriver_GetNumBRPS(m_nFd);
	}

	int _InternalGetNumWRPS() {
		return _hwbpProcDriver_GetNumWRPS(m_nFd);
	}

	uint64_t _InternalInstProcessHwBp(uint64_t hProcess, uint64_t lpBaseAddress, unsigned int hwbpLen, unsigned int hwbpType) {
		return _hwbpProcDriver_InstProcessHwBp(m_nFd, hProcess, lpBaseAddress, hwbpLen, hwbpType);
	}

	BOOL _InternalUninstProcessHwBp(uint64_t hHwbp) {
		return _hwbpProcDriver_UninstProcessHwBp(m_nFd, hHwbp);
	}

	BOOL _InternalSuspendProcessHwBp(uint64_t hHwbp) {
		return _hwbpProcDriver_SuspendProcessHwBp(m_nFd, hHwbp);
	}

	BOOL _InternalResumeProcessHwBp(uint64_t hHwbp) {
		return _hwbpProcDriver_ResumeProcessHwBp(m_nFd, hHwbp);
	}

	BOOL _InternalReadHwBpInfo(uint64_t hHwbp, uint64_t & nHitTotalCount, std::vector<HW_HIT_ITEM> & vOutput) {
		return _hwbpProcDriver_ReadHwBpInfo(m_nFd, hHwbp, nHitTotalCount, vOutput);
	}

	enum {
		CMD_OPEN_PROCESS, 				// 打开进程
		CMD_CLOSE_PROCESS, 				// 关闭进程
		CMD_GET_NUM_BRPS, 				// 获取CPU硬件执行断点支持数量
		CMD_GET_NUM_WRPS, 				// 获取CPU硬件访问断点支持数量
		CMD_INST_PROCESS_HWBP,			// 安装进程硬件断点
		CMD_UNINST_PROCESS_HWBP,		// 卸载进程硬件断点
		CMD_SUSPEND_PROCESS_HWBP,		// 暂停进程硬件断点
		CMD_RESUME_PROCESS_HWBP,		// 恢复进程硬件断点
		CMD_GET_HWBP_HIT_COUNT,			// 获取硬件断点命中地址数量
		CMD_GET_HWBP_HIT_DETAIL,		// 获取硬件断点命中详细信息
		CMD_SET_HOOK_PC,				// 设置无条件Hook跳转
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
	#pragma pack(pop)
	inline ssize_t _hwbpProcDriver_MyIoctl(
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
		std::memcpy(buf, pBuf + headerSize, bufSize);
		return outRead;
	}

	int _hwbpProcDriver_Connect(const std::string& procNodeAuthKey) {
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

	BOOL _hwbpProcDriver_Disconnect(int nFd) {
		if (nFd < 0) { return FALSE; }
		close(nFd);
		return TRUE;
	}

	BOOL _hwbpProcDriver_HideKernelModule(int nFd) {
		if (nFd < 0) { return FALSE; }
		ssize_t res = _hwbpProcDriver_MyIoctl(nFd, CMD_HIDE_KERNEL_MODULE, 0, 0, 0, NULL, 0);
		if (res != 0) {
			return FALSE;
		}
		return TRUE;
	}

	uint64_t _hwbpProcDriver_OpenProcess(int nFd, uint64_t pid) {
		if (nFd < 0 || pid == 0) { return 0; }
		uint64_t handle = 0;
		ssize_t res = _hwbpProcDriver_MyIoctl(nFd, CMD_OPEN_PROCESS, pid, 0, 0, (char*)&handle, sizeof(handle));
		if (res != 0) {
			return 0;
		}
		return handle;
	}

	BOOL _hwbpProcDriver_CloseHandle(int nFd, uint64_t hProcess) {
		if (nFd < 0 || !hProcess) { return FALSE; }
		if (_hwbpProcDriver_MyIoctl(nFd, CMD_CLOSE_PROCESS, hProcess, 0, 0, NULL, 0) != 0) {
			return FALSE;
		}
		return TRUE;
	}

	int _hwbpProcDriver_GetNumBRPS(int nFd) {
		if (nFd < 0) { return 0; }
		ssize_t res = _hwbpProcDriver_MyIoctl(nFd, CMD_GET_NUM_BRPS, 0, 0, 0, NULL, 0);
		return res;
	}

	int _hwbpProcDriver_GetNumWRPS(int nFd) {
		if (nFd < 0) { return 0; }
		ssize_t res = _hwbpProcDriver_MyIoctl(nFd, CMD_GET_NUM_WRPS, 0, 0, 0, NULL, 0);
		return res;
	}

	uint64_t _hwbpProcDriver_InstProcessHwBp(
		int nFd,
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		unsigned int hwbpLen,
		unsigned int hwbpType
	) {
		if (nFd < 0 || !hProcess || !lpBaseAddress) { return 0; }
		uint64_t param3 = 0;
		char *p = (char*)&param3;
		p[0] = hwbpLen;
		p[1] = hwbpType;
		
		uint64_t hHwbp;
		ssize_t res = _hwbpProcDriver_MyIoctl(nFd, CMD_INST_PROCESS_HWBP, hProcess, lpBaseAddress, param3, (char*)&hHwbp, sizeof(hHwbp));
		if (res != 0) {
			//printf("InstProcessHwBp _hwbpProcDriver_MyIoctl():%s\n", strerror(errno));
			return 0;
		}
		return hHwbp;
	}


	BOOL _hwbpProcDriver_UninstProcessHwBp(
		int nFd,
		uint64_t hHwbp) {
		if (nFd < 0 || !hHwbp) { return FALSE; }
		ssize_t res = _hwbpProcDriver_MyIoctl(nFd, CMD_UNINST_PROCESS_HWBP, hHwbp, 0, 0, NULL, 0);
		if (res != 0) {
			//printf("UninstProcessHwBp _hwbpProcDriver_MyIoctl():%s\n", strerror(errno));
			return FALSE;
		}
		return TRUE;
	}

	BOOL _hwbpProcDriver_SuspendProcessHwBp(
		int nFd,
		uint64_t hHwbp) {
		if (nFd < 0 || !hHwbp) { return FALSE; }

		ssize_t res = _hwbpProcDriver_MyIoctl(nFd, CMD_SUSPEND_PROCESS_HWBP, hHwbp, 0, 0, NULL, 0);
		if (res != 0) {
			//printf("SuspendProcessHwBp _hwbpProcDriver_MyIoctl():%s\n", strerror(errno));
			return FALSE;
		}
		return TRUE;
	}

	BOOL _hwbpProcDriver_ResumeProcessHwBp(
		int nFd,
		uint64_t hHwbp) {
		if (nFd < 0 || !hHwbp) { return FALSE; }

		ssize_t res = _hwbpProcDriver_MyIoctl(nFd, CMD_RESUME_PROCESS_HWBP, hHwbp, 0, 0, NULL, 0);
		if (res != 0) {
			//printf("ResumeProcessHwBp _hwbpProcDriver_MyIoctl():%s\n", strerror(errno));
			return FALSE;
		}
		return TRUE;
	}

	BOOL _hwbpProcDriver_ReadHwBpInfo(
		int nFd,
		uint64_t hHwbp,
		uint64_t & nHitTotalCount,
		std::vector<HW_HIT_ITEM> &vOutput
	) {
		if (nFd < 0 || !hHwbp) { return FALSE; }
		#pragma pack(1)
		struct {
			uint64_t nHitTotalCount;
			uint64_t nHitItemArrCount;
		} userData = {0};
		#pragma pack()
		ssize_t res = _hwbpProcDriver_MyIoctl(nFd, CMD_GET_HWBP_HIT_COUNT, hHwbp, 0, 0, (char*)&userData, sizeof(userData));
		//printf("ioctl res %d\n", res);
		if (res != 0) {
			//printf("ioctl():%s\n", strerror(errno));
			return FALSE;
		}
		nHitTotalCount = userData.nHitTotalCount;
		//printf("nHitTotalCount:%lu, nHitItemArrCount:%lu\n", userData.nHitTotalCount, userData.nHitItemArrCount);
		if(userData.nHitItemArrCount > 0) {
			std::vector<char> big_buf;
			big_buf.resize(sizeof(struct HW_HIT_ITEM) * userData.nHitItemArrCount);
			
			res = _hwbpProcDriver_MyIoctl(nFd, CMD_GET_HWBP_HIT_DETAIL, hHwbp, 0, 0, (char*)big_buf.data(), big_buf.size());
			//printf("ioctl res %d\n", res);
			if (res == 0) {
				//printf("ioctl():%s\n", strerror(errno));
				return FALSE;
			}
			auto* items = reinterpret_cast<const HW_HIT_ITEM*>(big_buf.data());
			size_t count = userData.nHitItemArrCount;
			vOutput.insert(vOutput.end(), items, items + count);
		}
		return TRUE;
	}

	
	BOOL _hwbpProcDriver_SetHookPC(
		int nFd,
		uint64_t pc
	) {
		if (nFd < 0) { return 0; }
		ssize_t res = _hwbpProcDriver_MyIoctl(nFd, CMD_SET_HOOK_PC, pc, 0, 0, NULL, 0);
		if (res != 0) {
			//printf("ioctl():%s\n", strerror(errno));
			return FALSE;
		}
		return TRUE;
	}
	
private:
	int m_nFd = -1;
};
#endif
#endif /* HW_BREAKPOINT_MANAGER_H_ */
