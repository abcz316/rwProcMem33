#ifndef MEM_SEARCH_KIT_COMPARE_VALUE_H_
#define MEM_SEARCH_KIT_COMPARE_VALUE_H_
#include <memory>
#include <vector>
#ifdef __linux__
#include <unistd.h>
#endif

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
namespace MemorySearchKit {
	namespace CompareValue {
		/*
		寻找精确数值
		使用方法:
		dwAddr：要搜索的缓冲区地址
		dwLen：要搜索的缓冲区大小
		value: 要搜索的数值
		nScanAlignBytesCount扫描的对齐字节数
		vOutputAddr：存放搜索完成的结果地址
		*/
		template<typename T> static inline void FindValue(size_t dwAddr, size_t dwLen, T value, size_t nScanAlignBytesCount, std::vector<size_t> & vOutputAddr) {
			vOutputAddr.clear();

			for (size_t i = 0; i < dwLen; i += nScanAlignBytesCount) {
				if ((dwLen - i) < sizeof(T)) {
					//要搜索的数据已经小于T的长度了
					break;
				}

				T* pData = (T*)(dwAddr + i);

				if (*pData == value) {
					vOutputAddr.push_back((size_t)((size_t)dwAddr + (size_t)i));
				}
				continue;
			}
		}

		/*
		寻找大于的数值
		使用方法:
		dwAddr：要搜索的缓冲区地址
		dwLen：要搜索的缓冲区大小
		value: 要搜索大于的数值
		nScanAlignBytesCount扫描的对齐字节数
		vOutputAddr：存放搜索完成的结果地址
		*/
		template<typename T> static inline void FindGreater(size_t dwAddr, size_t dwLen, T value, size_t nScanAlignBytesCount, std::vector<size_t> & vOutputAddr) {
			vOutputAddr.clear();

			for (size_t i = 0; i < dwLen; i += nScanAlignBytesCount) {
				if ((dwLen - i) < sizeof(T)) {
					//要搜索的数据已经小于T的长度了
					break;
				}

				T* pData = (T*)(dwAddr + i);
				if (*pData > value) {
					vOutputAddr.push_back((size_t)((size_t)dwAddr + (size_t)i));
				}
				continue;
			}
		}



		/*
		寻找小于的数值
		使用方法:
		dwAddr：要搜索的缓冲区地址
		dwLen：要搜索的缓冲区大小
		value: 要搜索小于的char数值
		nScanAlignBytesCount扫描的对齐字节数
		vOutputAddr：存放搜索完成的结果地址
		*/
		template<typename T> static inline void FindLess(size_t dwAddr, size_t dwLen, T value, size_t nScanAlignBytesCount, std::vector<size_t> & vOutputAddr) {
			vOutputAddr.clear();

			for (size_t i = 0; i < dwLen; i += nScanAlignBytesCount) {
				if ((dwLen - i) < sizeof(T)) {
					//要搜索的数据已经小于T的长度了
					break;
				}

				T* pData = (T*)(dwAddr + i);
				if (*pData < value) {
					vOutputAddr.push_back((size_t)((size_t)dwAddr + (size_t)i));
				}
				continue;
			}
		}
		/*
		寻找两者之间的数值
		使用方法:
		dwAddr：要搜索的缓冲区地址
		dwLen：要搜索的缓冲区大小
		value1: 要搜索的数值1
		value2: 要搜索的数值2
		nScanAlignBytesCount扫描的对齐字节数
		vOutputAddr：存放搜索完成的结果地址
		*/
		template<typename T> static inline void FindBetween(size_t dwAddr, size_t dwLen, T value1, T value2, size_t nScanAlignBytesCount, std::vector<size_t> & vOutputAddr) {
			vOutputAddr.clear();

			for (size_t i = 0; i < dwLen; i += nScanAlignBytesCount) {
				if ((dwLen - i) < sizeof(T)) {
					//要搜索的数据已经小于T的长度了
					break;
				}

				T* pData = (T*)(dwAddr + i);
				T cData = *pData;

				if (cData >= value1 && cData <= value2) {
					vOutputAddr.push_back((size_t)((size_t)dwAddr + (size_t)i));
				}
				continue;
			}
		}

		/*
		寻找增加的数值
		使用方法:
		dwOldAddr：要搜索的旧数据缓冲区地址
		dwNewAddr：要搜索的新数据缓冲区地址
		dwLen：要搜索的缓冲区大小
		nScanAlignBytesCount扫描的对齐字节数
		vOutputAddr：存放搜索完成的结果地址
		*/
		template<typename T> static inline void FindUnknowAdd(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nScanAlignBytesCount, std::vector<size_t> & vOutputAddr) {
			vOutputAddr.clear();

			for (size_t i = 0; i < dwLen; i += nScanAlignBytesCount) {
				if ((dwLen - i) < sizeof(T)) {
					//要搜索的数据已经小于T的长度了
					break;
				}

				T* pOldData = (T*)(dwOldAddr + i);
				T* pNewData = (T*)(dwNewAddr + i);
				if (*pNewData > *pOldData) {
					vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));
				}
				continue;
			}
		}

		/*
		寻找减少的数值
		使用方法:
		dwOldAddr：要搜索的旧数据缓冲区地址
		dwNewAddr：要搜索的新数据缓冲区地址
		dwLen：要搜索的缓冲区大小
		nScanAlignBytesCount扫描的对齐字节数
		vOutputAddr：存放搜索完成的结果地址
		*/
		template<typename T> static inline void FindUnknowSum(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nScanAlignBytesCount, std::vector<size_t> & vOutputAddr) {
			vOutputAddr.clear();

			for (size_t i = 0; i < dwLen; i += nScanAlignBytesCount) {
				if ((dwLen - i) < sizeof(T)) {
					//要搜索的数据已经小于T的长度了
					break;
				}

				T* pOldData = (T*)(dwOldAddr + i);
				T* pNewData = (T*)(dwNewAddr + i);
				if (*pNewData < *pOldData) {
					vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));
				}
				continue;
			}
		}
	
		/*
		寻找变动的数值
		使用方法:
		dwOldAddr：要搜索的旧数据缓冲区地址
		dwNewAddr：要搜索的新数据缓冲区地址
		dwLen：要搜索的缓冲区大小
		nScanAlignBytesCount扫描的对齐字节数
		vOutputAddr：存放搜索完成的结果地址
		*/
		template<typename T> static inline void FindChanged(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nScanAlignBytesCount, std::vector<size_t> & vOutputAddr) {
			vOutputAddr.clear();

			for (size_t i = 0; i < dwLen; i += nScanAlignBytesCount) {
				if ((dwLen - i) < sizeof(T)) {
					//要搜索的数据已经小于T的长度了
					break;
				}

				T* pOldData = (T*)(dwOldAddr + i);
				T* pNewData = (T*)(dwNewAddr + i);
				if (*pNewData != *pOldData) {
					vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));
				}
				continue;
			}
		}
		/*
		寻找未变动的数值
		使用方法:
		dwOldAddr：要搜索的旧数据缓冲区地址
		dwNewAddr：要搜索的新数据缓冲区地址
		dwLen：要搜索的缓冲区大小
		nScanAlignBytesCount扫描的对齐字节数
		vOutputAddr：存放搜索完成的结果地址
		*/
		template<typename T> static inline void FindNoChange(size_t dwOldAddr, size_t dwNewAddr, size_t dwLen, size_t nScanAlignBytesCount, std::vector<size_t> & vOutputAddr) {
			vOutputAddr.clear();

			for (size_t i = 0; i < dwLen; i += nScanAlignBytesCount) {
				if ((dwLen - i) < sizeof(T)) {
					//要搜索的数据已经小于T的长度了
					break;
				}

				T* pOldData = (T*)(dwOldAddr + i);
				T* pNewData = (T*)(dwNewAddr + i);
				if (*pNewData == *pOldData) {
					vOutputAddr.push_back((size_t)((size_t)dwOldAddr + (size_t)i));
				}
				continue;
			}
		}



		/*
		寻找字节集
		dwAddr: 要搜索的缓冲区地址
		dwLen: 要搜索的缓冲区长度
		bMask: 要搜索的字节集
		szMask: 要搜索的字节集模糊码（xx=已确定字节，??=可变化字节，支持x?、?x）
		nMaskLen: 要搜索的字节集长度
		nScanAlignBytesCount扫描的对齐字节数
		vOutputAddr: 搜索出来的结果地址存放数组
		使用方法：
		std::vector<size_t> vAddr;
		FindFeaturesBytes((size_t)lpBuffer, dwBufferSize, (PBYTE)"\x8B\xE8\x00\x00\x00\x00\x33\xC0\xC7\x06\x00\x00\x00\x00\x89\x86\x40", "xxxx??xx?xx?xxxxx",17,1,vAddr);
		*/
		static inline void FindFeaturesBytes(size_t dwAddr, size_t dwLen, unsigned char *bMask, const char* szMask, size_t nMaskLen, size_t nScanAlignBytesCount, std::vector<size_t> & vOutputAddr) {
			vOutputAddr.clear();
			for (size_t i = 0; i < dwLen; i += nScanAlignBytesCount) {
				if ((dwLen - i) < nMaskLen) {
					//要搜索的数据已经小于特征码的长度了
					break;
				}
				unsigned char* pData = (unsigned char*)(dwAddr + i);
				unsigned char*bTemMask = bMask;
				const char* szTemMask = szMask;

				bool bContinue = false;
				for (; *szTemMask; szTemMask += 2, ++pData, ++bTemMask) {
					if ((*szTemMask == 'x') && (*(szTemMask + 1) == 'x') && ((*pData) != (*bTemMask))) {
						bContinue = true;
						break;
					} else if ((*szTemMask == 'x') && (*(szTemMask + 1) == '?') && ((((*pData) >> 4) & 0xFu) != (((*bTemMask) >> 4) & 0xFu))) {
						bContinue = true;
						break;
					} else if ((*szTemMask == '?') && (*(szTemMask + 1) == 'x') && (((*pData) & 0xFu) != ((*bTemMask) & 0xFu))) {
						bContinue = true;
						break;
					}
				}
				if (bContinue) {
					continue;
				}

				if ((*szTemMask) == '\x00') {
					vOutputAddr.push_back((size_t)((size_t)dwAddr + (size_t)i));
				}
			}
		}
		/*
		寻找字节集
		dwWaitSearchAddress: 要搜索的缓冲区地址
		dwLen: 要搜索的缓冲区长度
		bForSearch: 要搜索的字节集
		ifLen: 要搜索的字节集长度
		nScanAlignBytesCount扫描的对齐字节数
		vOutputAddr: 搜索出来的结果地址存放数组
		使用方法：
		std::vector<size_t> vAddr;
		FindFeaturesBytes((size_t)lpBuffer, dwBufferSize, (PBYTE)"\x8B\xE8\x00\x00\x00\x00\x33\xC0\xC7\x06\x00\x00\x00\x00\x89\x86\x40", 17,1,vAddr);
		*/
		static inline void FindBytes(size_t dwWaitSearchAddress, size_t dwLen, unsigned char *bForSearch, size_t ifLen, size_t nScanAlignBytesCount, std::vector<size_t> & vOutputAddr) {
			for (size_t i = 0; i < dwLen; i += nScanAlignBytesCount) {
				if ((dwLen - i) < ifLen) {
					//要搜索的数据已经小于特征码的长度了
					break;
				}
				unsigned char* pData = (unsigned char*)(dwWaitSearchAddress + i);
				unsigned char*bTemForSearch = bForSearch;

				bool bContinue = false;
				for (size_t y = 0; y < ifLen; y++, ++pData, ++bTemForSearch) {
					if (*pData != *bTemForSearch) {
						bContinue = true;
						break;
					}
				}
				if (bContinue) {
					continue;
				}
				vOutputAddr.push_back((size_t)((size_t)dwWaitSearchAddress + (size_t)i));
			}
		}
	}
}

#endif /* MEM_SEARCH_KIT_COMPARE_VALUE_H_ */

