#ifndef IOCTL_BUFFER_POOL_H_
#define IOCTL_BUFFER_POOL_H_
#ifdef __linux__
#include <cstddef>  // for size_t
#include <new>      // for std::nothrow

// 创建一个静态内存缓冲区，避免重复分配内存
class IoctlBufferPool {
    static constexpr size_t kDefaultBuffer = 4096;   // 默认小缓冲区
    char    _smallBuf[kDefaultBuffer];               // 小缓冲区（无需动态分配）
    char*   _largeBuf     = nullptr;                 // 大缓冲区指针
    size_t  _largeBufSize = 0;                       // 已分配大缓冲区容量

public:
    ~IoctlBufferPool() {
        // 在线程退出时自动释放大缓冲区
		if(_largeBuf) {
        	delete[] _largeBuf;
		}
    }

    // 获取至少 capacity 字节的缓冲区
    char* getBuffer(size_t capacity) {
        if (capacity <= kDefaultBuffer) {
            return _smallBuf;
        }
        if (_largeBufSize < capacity) {
            // 需要更大的缓冲区，先释放旧的再分配
			if(_largeBuf) {
				delete[] _largeBuf;
			}
            _largeBuf = new (std::nothrow) char[capacity];
            _largeBufSize = _largeBuf ? capacity : 0;
        }
        return _largeBuf;
    }
};
#endif /*__linux__*/

#endif /* IOCTL_BUFFER_POOL_H_ */
