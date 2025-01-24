#ifndef HW_BREAKPOINT_MANAGER_H_
#define HW_BREAKPOINT_MANAGER_H_

#define HWBP_FILE_NODE "/dev/hwbpProc3"

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
#define MAJOR_NUM 100
#define IOCTL_HWBP_OPEN_PROCESS 				_IOR(MAJOR_NUM, 1, char*) //打开进程
#define IOCTL_HWBP_CLOSE_HANDLE 				_IOR(MAJOR_NUM, 2, char*) //关闭进程
#define IOCTL_HWBP_GET_NUM_BRPS 				_IOR(MAJOR_NUM, 3, char*) //获取CPU支持硬件执行断点的数量
#define IOCTL_HWBP_GET_NUM_WRPS 				_IOR(MAJOR_NUM, 4, char*) //获取CPU支持硬件访问断点的数量
#define IOCTL_HWBP_INST_PROCESS_HWBP			_IOR(MAJOR_NUM, 5, char*) //设置进程硬件断点
#define IOCTL_HWBP_UNINST_PROCESS_HWBP		_IOR(MAJOR_NUM, 6, char*) //删除进程硬件断点
#define IOCTL_HWBP_SUSPEND_PROCESS_HWBP	_IOR(MAJOR_NUM, 7, char*) //暂停进程硬件断点
#define IOCTL_HWBP_RESUME_PROCESS_HWBP		_IOR(MAJOR_NUM, 8, char*) //恢复进程硬件断点
#define IOCTL_HWBP_GET_HWBP_HIT_COUNT		_IOR(MAJOR_NUM, 9, char*) //获取硬件断点命中地址数量
#define IOCTL_HWBP_SET_HOOK_PC						_IOR(MAJOR_NUM, 20, char*)

class CHwBreakpointMgr {
public:

	CHwBreakpointMgr() {}
	~CHwBreakpointMgr() { DisconnectDriver(); }

	//连接驱动（驱动节点文件路径名，是否使用躲避SELinux的通信方式，错误代码），返回值：驱动连接句柄，>=0代表成功
	BOOL ConnectDriver(const char* lpszDriverFileNodePath, BOOL bUseBypassSELinuxMode, int & err) {
		return _InternalConnectDriver(lpszDriverFileNodePath, bUseBypassSELinuxMode, err);
	}

	//断开驱动，返回值：TRUE成功，FALSE失败
	BOOL DisconnectDriver() {
		return _InternalDisconnectDriver();
	}

	//驱动是否连接正常，返回值：TRUE已连接，FALSE未连接
	BOOL IsDriverConnected() {
		return _InternalIsDriverConnected();
	}
	
	//驱动_打开进程（进程PID），返回值：进程句柄，0为失败
	uint64_t OpenProcess(uint64_t pid) {
		return _InternalOpenProcess(pid);
	}

	//驱动_关闭进程（进程句柄），返回值：TRUE成功，FALSE失败
	BOOL CloseHandle(uint64_t hProcess) {
		return _InternalCloseHandle(hProcess);
	}

	//驱动_获取CPU支持硬件执行断点的数量
	int GetNumBRPS() {
		return _InternalGetNumBRPS();
	}

	//驱动_获取CPU支持硬件访问断点的数量
	int GetNumWRPS() {
		return _InternalGetNumWRPS();
	}

	//驱动_新增硬件断点，返回值：成功返回断点句柄，失败返回0
	uint64_t AddProcessHwBp(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		unsigned int hwbpLen,
		unsigned int hwbpType
	) {
		return _InternalAddProcessHwBp(hProcess, lpBaseAddress, hwbpLen, hwbpType);
	}


	//驱动_删除硬件断点，返回值：TRUE成功，FALSE失败
	BOOL DelProcessHwBp(uint64_t hHwbp) {
		return _InternalDelProcessHwBp(hHwbp);
	}

	//驱动_暂停硬件断点，返回值：TRUE成功，FALSE失败
	BOOL SuspendProcessHwBp(uint64_t hHwbp) {
		return _InternalSuspendProcessHwBp(hHwbp);
	}
	
	//驱动_恢复硬件断点，返回值：TRUE成功，FALSE失败
	BOOL ResumeProcessHwBp(uint64_t hHwbp) {
		return _InternalResumeProcessHwBp(hHwbp);
	}

	//驱动_读取硬件断点命中记录信息，返回值：TRUE成功，FALSE失败
	BOOL ReadHwBpInfo(uint64_t hHwbp, uint64_t & nHitTotalCount, std::vector<HW_HIT_ITEM> & vOutput) {
		return _InternalReadHwBpInfo(hHwbp, nHitTotalCount, vOutput);
	}

	//驱动_设置无条件Hook跳转（pc：硬件执行断点触发后，要跳转到的进程内存地址），返回值：TRUE成功，FALSE失败
	BOOL SetHookPC(uint64_t pc) {
		return _hwbpProcDriver_SetHookPC(m_nDriverLink, pc);
	}
	
private:
	BOOL _InternalConnectDriver(const char* lpszDriverFileNodePath, BOOL bUseBypassSELinuxMode, int& err) {
		if (m_nDriverLink >= 0) { return TRUE; }
		m_nDriverLink = _hwbpProcDriver_Connect(lpszDriverFileNodePath);
		if (m_nDriverLink < 0) {
			err = m_nDriverLink;
			return FALSE;
		}
		_hwbpProcDriver_SetUseBypassSELinuxMode(bUseBypassSELinuxMode);
		err = 0;
		return TRUE;
	}

	BOOL _InternalDisconnectDriver() {
		if (m_nDriverLink >= 0) {
			_hwbpProcDriver_Disconnect(m_nDriverLink);
			m_nDriverLink = -1;
			return TRUE;
		}
		return FALSE;
	}

	BOOL _InternalIsDriverConnected() {
		return m_nDriverLink >= 0 ? TRUE : FALSE;
	}

	uint64_t _InternalOpenProcess(uint64_t pid) {
		return _hwbpProcDriver_OpenProcess(m_nDriverLink, pid);
	}

	BOOL _InternalCloseHandle(uint64_t hProcess) {
		return _hwbpProcDriver_CloseHandle(m_nDriverLink, hProcess);
	}

	int _InternalGetNumBRPS() {
		return _hwbpProcDriver_GetNumBRPS(m_nDriverLink);
	}

	int _InternalGetNumWRPS() {
		return _hwbpProcDriver_GetNumWRPS(m_nDriverLink);
	}

	uint64_t _InternalAddProcessHwBp(uint64_t hProcess, uint64_t lpBaseAddress, unsigned int hwbpLen, unsigned int hwbpType) {
		return _hwbpProcDriver_InstProcessHwBp(m_nDriverLink, hProcess, lpBaseAddress, hwbpLen, hwbpType);
	}

	BOOL _InternalDelProcessHwBp(uint64_t hHwbp) {
		return _hwbpProcDriver_UninstProcessHwBp(m_nDriverLink, hHwbp);
	}

	BOOL _InternalSuspendProcessHwBp(uint64_t hHwbp) {
		return _hwbpProcDriver_SuspendProcessHwBp(m_nDriverLink, hHwbp);
	}

	BOOL _InternalResumeProcessHwBp(uint64_t hHwbp) {
		return _hwbpProcDriver_ResumeProcessHwBp(m_nDriverLink, hHwbp);
	}

	BOOL _InternalReadHwBpInfo(uint64_t hHwbp, uint64_t & nHitTotalCount, std::vector<HW_HIT_ITEM> & vOutput) {
		return _hwbpProcDriver_ReadHwBpInfo(m_nDriverLink, hHwbp, nHitTotalCount, vOutput);
	}
	
	void _InternalSetUseBypassSELinuxMode(BOOL bUseBypassSELinuxMode) {
		return _hwbpProcDriver_SetUseBypassSELinuxMode(bUseBypassSELinuxMode);
	}

	int _hwbpProcDriver_MyIoctl(int fd, unsigned int cmd, unsigned long buf, unsigned long bufSize) {
		if (m_bUseBypassSELinuxMode == TRUE) {
			//驱动通信方式：lseek，躲开系统SELinux拦截
			char *lseekBuf = (char*)malloc(sizeof(cmd) + bufSize);
			*(unsigned long*)lseekBuf = cmd;
			memcpy((void*)((size_t)lseekBuf + (size_t)sizeof(cmd)), (void*)buf, bufSize);
			uint64_t ret = lseek64(fd, (off64_t)lseekBuf, SEEK_CUR);
			memcpy((void*)buf, (void*)((size_t)lseekBuf + (size_t)sizeof(cmd)), bufSize);
			free(lseekBuf);
			return ret;
		} else {
			//驱动通信方式：ioctl
			return ioctl(fd, cmd, buf);
		}
	}

	int _hwbpProcDriver_Connect(const char * lpszDriverFileNodePath) {
		int nDriverLink = open(lpszDriverFileNodePath, O_RDWR);
		if (nDriverLink < 0) {
			int last_err = errno;
			if (last_err == EACCES) {
				chmod(lpszDriverFileNodePath, 666);
				nDriverLink = open(lpszDriverFileNodePath, O_RDWR);
				last_err = errno;
				chmod(lpszDriverFileNodePath, 0600);
			}
			if (nDriverLink < 0) {
				//printf("open error():%s\n", strerror(last_err));
				return -last_err;
			}
		}
		return nDriverLink;
	}

	BOOL _hwbpProcDriver_Disconnect(int nDriverLink) {
		if (nDriverLink < 0) { return FALSE; }
		close(nDriverLink);
		return TRUE;
	}

	uint64_t _hwbpProcDriver_OpenProcess(int nDriverLink, uint64_t pid) {
		if (nDriverLink < 0) { return 0; }
		char buf[8] = { 0 };
		memcpy(buf, &pid, 8);
		int res = _hwbpProcDriver_MyIoctl(nDriverLink, IOCTL_HWBP_OPEN_PROCESS, (unsigned long)&buf, sizeof(buf));
		if (res != 0) {
			//printf("OpenProcess _hwbpProcDriver_MyIoctl():%s\n", strerror(errno));
			return 0;
		}
		uint64_t ptr = *(uint64_t*)&buf[0];
		return ptr;
	}


	BOOL _hwbpProcDriver_CloseHandle(int nDriverLink, uint64_t handle) {
		if (nDriverLink < 0 || !handle) { return FALSE; }
		int res = _hwbpProcDriver_MyIoctl(nDriverLink, IOCTL_HWBP_CLOSE_HANDLE, (unsigned long)&handle, sizeof(handle));
		if (res != 0) {
			//printf("CloseHandle _hwbpProcDriver_MyIoctl():%s\n", strerror(errno));
			return FALSE;
		}
		return TRUE;
	}

	int _hwbpProcDriver_GetNumBRPS(int nDriverLink) {
		if (nDriverLink < 0) { return 0; }
		int res = _hwbpProcDriver_MyIoctl(nDriverLink, IOCTL_HWBP_GET_NUM_BRPS, 0, 0);
		return res;
	}
	int _hwbpProcDriver_GetNumWRPS(int nDriverLink) {
		if (nDriverLink < 0) { return 0; }
		int res = _hwbpProcDriver_MyIoctl(nDriverLink, IOCTL_HWBP_GET_NUM_WRPS, 0, 0);
		return res;
	}

	uint64_t _hwbpProcDriver_InstProcessHwBp(
		int nDriverLink,
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		unsigned int hwbpLen,
		unsigned int hwbpType
	) {
		if (nDriverLink < 0 || !hProcess || !lpBaseAddress) { return 0; }
		#pragma pack(1)
		struct {
			uint64_t hProcess;
			uint64_t lpBaseAddress;
			unsigned int hwbpLen;
			unsigned int hwbpType;
		} userData = {0};
		#pragma pack()
		userData.hProcess = hProcess;
		userData.lpBaseAddress = lpBaseAddress;
		userData.hwbpLen = hwbpLen;
		userData.hwbpType = hwbpType;
	
		int res = _hwbpProcDriver_MyIoctl(nDriverLink, IOCTL_HWBP_INST_PROCESS_HWBP, (unsigned long)&userData, sizeof(userData));
		if (res != 0) {
			//printf("InstProcessHwBp _hwbpProcDriver_MyIoctl():%s\n", strerror(errno));
			return 0;
		}
		return userData.hProcess;
	}


	BOOL _hwbpProcDriver_UninstProcessHwBp(
		int nDriverLink,
		uint64_t hHwbp) {
		if (nDriverLink < 0 || !hHwbp) { return FALSE; }

		int res = _hwbpProcDriver_MyIoctl(nDriverLink, IOCTL_HWBP_UNINST_PROCESS_HWBP, (unsigned long)&hHwbp, sizeof(hHwbp));
		if (res != 0) {
			//printf("UninstProcessHwBp _hwbpProcDriver_MyIoctl():%s\n", strerror(errno));
			return FALSE;
		}
		return TRUE;
	}

	BOOL _hwbpProcDriver_SuspendProcessHwBp(
		int nDriverLink,
		uint64_t hHwbp) {
		if (nDriverLink < 0 || !hHwbp) { return FALSE; }

		int res = _hwbpProcDriver_MyIoctl(nDriverLink, IOCTL_HWBP_SUSPEND_PROCESS_HWBP, (unsigned long)&hHwbp, sizeof(hHwbp));
		if (res != 0) {
			//printf("SuspendProcessHwBp _hwbpProcDriver_MyIoctl():%s\n", strerror(errno));
			return FALSE;
		}
		return TRUE;
	}

	BOOL _hwbpProcDriver_ResumeProcessHwBp(
		int nDriverLink,
		uint64_t hHwbp) {
		if (nDriverLink < 0 || !hHwbp) { return FALSE; }

		int res = _hwbpProcDriver_MyIoctl(nDriverLink, IOCTL_HWBP_RESUME_PROCESS_HWBP, (unsigned long)&hHwbp, sizeof(hHwbp));
		if (res != 0) {
			//printf("ResumeProcessHwBp _hwbpProcDriver_MyIoctl():%s\n", strerror(errno));
			return FALSE;
		}
		return TRUE;
	}

	BOOL _hwbpProcDriver_ReadHwBpInfo(
		int nDriverLink,
		uint64_t hHwbp,
		uint64_t & nHitTotalCount,
		std::vector<HW_HIT_ITEM> &vOutput
	) {
		if (nDriverLink < 0 || !hHwbp) { return FALSE; }
		#pragma pack(1)
		struct {
			uint64_t hHwbp;
			uint64_t nHitTotalCount;
			uint64_t nHitItemArrCount;
		} userData = {0};
		#pragma pack()
		userData.hHwbp = hHwbp;
		int res = _hwbpProcDriver_MyIoctl(nDriverLink, IOCTL_HWBP_GET_HWBP_HIT_COUNT, (unsigned long)&userData, sizeof(userData));
		//printf("ioctl res %d\n", res);
		if (res != 0) {
			//printf("ioctl():%s\n", strerror(errno));
			return FALSE;
		}
		nHitTotalCount = userData.nHitTotalCount;
		//printf("nHitTotalCount:%lu, nHitItemArrCount:%lu\n", userData.nHitTotalCount, userData.nHitItemArrCount);
		if(userData.nHitItemArrCount > 0) {
			std::vector<char> big_buf(sizeof(struct HW_HIT_ITEM) * userData.nHitItemArrCount);
			memcpy(big_buf.data(), &hHwbp, sizeof(hHwbp));

			res = read(nDriverLink, big_buf.data(), big_buf.size());
			//printf("read res %d\n", res);
			if (res < 0) {
				return FALSE;
			}

			size_t itemSize = sizeof(HW_HIT_ITEM);
			for (size_t i = 0; i < big_buf.size(); i += itemSize) {
				HW_HIT_ITEM item;
				memcpy(&item, big_buf.data() + i, itemSize);
				vOutput.push_back(item);
			}
		}
		return TRUE;
	}

	
	BOOL _hwbpProcDriver_SetHookPC(
		int nDriverLink,
		uint64_t pc
	) {
		if (nDriverLink < 0) { return 0; }
		int res = _hwbpProcDriver_MyIoctl(nDriverLink, IOCTL_HWBP_SET_HOOK_PC, (unsigned long)&pc, sizeof(pc));
		if (res != 0) {
			//printf("ioctl():%s\n", strerror(errno));
			return FALSE;
		}
		return TRUE;
	}

	void _hwbpProcDriver_SetUseBypassSELinuxMode(BOOL bUseBypassSELinuxMode) {
		m_bUseBypassSELinuxMode = bUseBypassSELinuxMode;
	}
	
private:
	int m_nDriverLink = -1;
	BOOL m_bUseBypassSELinuxMode = FALSE;
};
#endif
#endif /* HW_BREAKPOINT_MANAGER_H_ */
