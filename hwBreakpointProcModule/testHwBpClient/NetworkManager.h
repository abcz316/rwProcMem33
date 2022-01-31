#pragma once
#include <iostream>
#include <vector>
#include<WINSOCK2.H>
#pragma comment(lib,"ws2_32.lib")

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
struct USER_HIT_INFO {
	size_t hit_addr; //命中地址
	size_t hit_count; //命中次数
	struct my_user_pt_regs regs; //最后一次命中的寄存器数据
};
struct HIT_CONDITIONS {
	char enable_regs[31];
	char enable_sp;
	char enable_pc;
	char enable_pstate;
	char enable_orig_x0;
	char enable_syscallno;
	struct my_user_pt_regs regs;
};
#pragma pack()

class CNetworkManager {
public:
	CNetworkManager();
	~CNetworkManager();
	bool ConnectHwBpServer(std::string ip, int port);
	bool IsConnected();
	void Disconnect();
	bool Reconnect();

	bool SetHwBpHitConditions(HIT_CONDITIONS hitConditions);

	bool AddProcessHwBp(
		uint32_t pid,
		uint64_t address,
		uint32_t hwBpAddrLen,
		uint32_t hwBpAddrType,
		uint32_t hwBpThreadType,
		uint32_t hwBpKeepTimeMs,
		uint32_t & all_task_count,
		uint32_t & hwbp_success_task,
		std::vector<USER_HIT_INFO> & vHitResult);
private:
	std::string m_ip;
	int m_port;
	SOCKET m_skServer = NULL;

};

