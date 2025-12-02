#include "pch.h"
#include "NetworkMgr.h"
#include "Global.h"
#include "../testHwBp/jni/HwBreakpointMgr4.h"

CNetworkMgr::CNetworkMgr() {
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);
	m_skServer = NULL;
}
CNetworkMgr::~CNetworkMgr() {
	if (m_skServer) {
		closesocket(m_skServer);
	}
	WSACleanup();
}
bool CNetworkMgr::ConnectHwBpServer(std::string ip, int port) {
	m_ip = ip;
	m_port = port;

	if (m_skServer) {
		closesocket(m_skServer);
		m_skServer = NULL;
	}

	if (m_ip.empty() || port == 0) {
		return false;
	}
	m_skServer = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
	if (connect(m_skServer, (sockaddr*)&addr, sizeof(sockaddr_in)) != 0) {
		closesocket(m_skServer);
		m_skServer = NULL;
		return false;
	}
	return true;
}
bool CNetworkMgr::IsConnected() {
	return m_skServer ? true : false;
}
void CNetworkMgr::Disconnect() {
	if (m_skServer) {
		closesocket(m_skServer);
		m_skServer = NULL;
	}
}
bool CNetworkMgr::Reconnect() {
	return ConnectHwBpServer(m_ip, m_port);
}

bool CNetworkMgr::InstProcessHwBp(const InstProcessHwBpInfo & input, InstProcessHwBpResult & output) {
	output.allTaskCount = 0;
	output.hwbpInstalledCount = 0;
	output.vThreadHit.clear();
	if (!m_skServer) {
		return false;
	}

	unsigned char command = CMD_ADDPROCESSHWBP;
	if (sendall(m_skServer, (char*)&command, 1, 0) <= 0) {
		closesocket(m_skServer);
		m_skServer = NULL;
		return false;
	}

	if (sendall(m_skServer, (void*)&input, sizeof(input), 0) > 0) {
#pragma pack(1)
		struct {
			uint32_t allTaskCount;
			uint32_t hwbpInstalledCount;
			uint32_t hitThreadCount;
		} result;
#pragma pack()
		if (recvall(m_skServer, &result, sizeof(result), MSG_WAITALL) > 0) {
			for (; result.hitThreadCount > 0; result.hitThreadCount--) {

#pragma pack(1)
				struct {
					uint64_t taskId = 0;
					uint64_t address = 0;
					uint64_t hitTotalCount = 0;
					uint64_t saveHitItemCount = 0;
				} threadInfoBuf = { 0 };
#pragma pack()
				if (recvall(m_skServer, &threadInfoBuf, sizeof(threadInfoBuf), MSG_WAITALL) > 0) {
					InstProcessHwBpResultChild child;
					child.taskId = threadInfoBuf.taskId;
					child.address = threadInfoBuf.address;
					child.hitTotalCount = threadInfoBuf.hitTotalCount;
					for (; threadInfoBuf.saveHitItemCount > 0; threadInfoBuf.saveHitItemCount--) {
						HW_HIT_ITEM hitItem = { 0 };
						if (recvall(m_skServer, &hitItem, sizeof(HW_HIT_ITEM), MSG_WAITALL) > 0) {
							child.vHitItem.push_back(hitItem);
						}
					}
					output.vThreadHit.push_back(child);
				}
			}
			output.allTaskCount = result.allTaskCount;
			output.hwbpInstalledCount = result.hwbpInstalledCount;
			return true;
		}
	}
	return false;
}