# 驱动名称：Linux ARM64内核硬件进程内存读写驱动37
本驱动支持所有能解锁BL的手机，无论小米、黑鲨、红魔、ROG、一加、三星、摩托罗拉等等，并且不需要手机厂商开放内核源码。只需要手动修改五六处地方，就可以跑在任意机型的内核上！具体修改过程不再本文章的论述中。本文章仅提供驱动原始模板代码。

### 本驱动接口列表：
1. 驱动_设置驱动设备接口文件允许同时被使用的最大值: SetMaxDevFileOpen
2. 驱动_隐藏驱动（卸载驱动需重启机器）: HideKernelModule
3. 驱动_打开进程: OpenProcess
4. 驱动_读取进程内存: ReadProcessMemory
5. 驱动_写入进程内存: WriteProcessMemory
6. 驱动_关闭进程: CloseHandle
7. 驱动_获取进程内存块列表: VirtualQueryExFull（可选：显示全部内存、只显示在物理内存中的内存）
8. 驱动_获取进程PID列表: GetProcessPidList
9. 驱动_获取进程权限等级: GetProcessGroup
10.驱动_提升进程权限到Root: SetProcessRoot
11.驱动_获取进程占用物理内存大小: GetProcessRSS
12.驱动_获取进程命令行: GetProcessCmdline


# 驱动名称: Linux ARM64内核硬件断点进程调试驱动1
### 本驱动接口列表：
1.  驱动_打开进程: OpenProcess
2.  驱动_获取CPU支持硬件执行断点的数量: GetNumBRPS
3.  驱动_获取CPU支持硬件访问断点的数量: GetNumWRPS
4.  驱动_设置进程硬件断点: AddProcessHwBp
5.  驱动_删除进程硬件断点: DelProcessHwBp
6.  驱动_读取硬件断点命中信息: ReadHwBpInfo
7.  驱动_清空硬件断点命中信息: CleanHwBpInfo
8.  驱动_关闭进程: CloseHandle
