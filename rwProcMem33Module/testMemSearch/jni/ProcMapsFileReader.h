//
// Created by abcz316 on 2020/2/26.
//

#ifndef PROC_MAPS_FILE_READER_H
#define PROC_MAPS_FILE_READER_H
#include "../../testKo/jni/MemoryReaderWriter39.h"
#include <fstream>
#ifndef __linux__
#include <windows.h>
#endif
#include "MapRegionType.h"

class ProcMapsFileReader : public IMemReaderWriterProxy {
   public:
	ProcMapsFileReader(int _pid) : pid(_pid) {}
	~ProcMapsFileReader() {}
	BOOL VirtualQueryExFull(uint64_t hProcess,
							BOOL showPhy,
							std::vector<DRIVER_REGION_INFO>& vOutput) override {
		vOutput.clear();
		std::stringstream ss;
		ss << "/proc/" << pid << "/maps";
		std::string mapsPath = ss.str();

		std::ifstream mapsFile(mapsPath);
		if (!mapsFile.is_open()) {
			return FALSE;
		}

		std::string line;
		while (std::getline(mapsFile, line)) {
			std::stringstream linestream(line);

			std::string addrRange, perms, offset, dev, inode;
			std::string pathname;

			linestream >> addrRange >> perms >> offset >> dev >> inode;
			if (!linestream.eof()) {
				std::getline(linestream, pathname);
				if (!pathname.empty() && pathname[0] == ' ') {
					pathname.erase(0, 1);
				}
			}
			size_t dashPos = addrRange.find('-');
			if (dashPos == std::string::npos) {
				continue;
			}

			std::string startStr = addrRange.substr(0, dashPos);
			std::string endStr = addrRange.substr(dashPos + 1);

			uint64_t startAddr = std::stoull(startStr, nullptr, 16);
			uint64_t endAddr = std::stoull(endStr, nullptr, 16);

			DRIVER_REGION_INFO info = {0};
			info.baseaddress = startAddr;
			info.size = endAddr - startAddr;
			StringToMapsType(perms, info.protection, info.type);
			if (!pathname.empty()) {
				std::strncpy(info.name, pathname.c_str(),
							 sizeof(info.name) - 1);
				info.name[sizeof(info.name) - 1] = '\0';
			}

			vOutput.push_back(info);
		}

		mapsFile.close();
		return TRUE;
	}
	BOOL ReadProcessMemory(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void *lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesRead = NULL,
		BOOL bIsForceRead = FALSE) override { return FALSE; }
	BOOL WriteProcessMemory(
		uint64_t hProcess,
		uint64_t lpBaseAddress,
		void * lpBuffer,
		size_t nSize,
		size_t * lpNumberOfBytesWritten = NULL,
		BOOL bIsForceWrite = FALSE) override { return FALSE; }
	BOOL CheckProcessMemAddrValid(
		uint64_t hProcess,
		uint64_t lpBaseAddress) override { return FALSE; }
   private:
	int pid = 0;
};

#endif	// PROC_MAPS_FILE_READER_H
