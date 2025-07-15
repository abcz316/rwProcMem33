#pragma once
#include <iostream>
#include <vector>
#include<WINSOCK2.H>
#pragma comment(lib,"ws2_32.lib")
#include "../testHwBpServer/jni/hwbpserver.h"
class CNetworkMgr {
public:
	CNetworkMgr();
	~CNetworkMgr();
	bool ConnectHwBpServer(std::string ip, int port);
	bool IsConnected();
	void Disconnect();
	bool Reconnect();

	bool InstProcessHwBp(const InstProcessHwBpInfo& input, InstProcessHwBpResult& output);
private:
	std::string m_ip;
	int m_port;
	SOCKET m_skServer = NULL;

};

