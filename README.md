# 驱动1名称：Linux ARM64内核硬件进程内存读写驱动39
本驱动支持所有能解锁BL的手机，无论小米、黑鲨、红魔、ROG、一加、三星、摩托罗拉等等，并且不需要手机厂商开放内核源码。只需要手动修改五六处地方，就可以跑在任意机型的内核上！具体修改过程不再本文章的论述中。本文章仅提供驱动源代码。

### 本驱动接口列表：
1.  驱动_打开进程: OpenProcess
2.  驱动_读取进程内存: ReadProcessMemory
3.  驱动_写入进程内存: WriteProcessMemory
4.  驱动_关闭进程: CloseHandle
5.  驱动_获取进程内存块列表: VirtualQueryExFull（可选：显示全部内存、仅显示物理内存）
6.  驱动_获取进程PID列表: GetPidList
7.  驱动_提升进程权限到Root: SetProcessRoot
8.  驱动_获取进程占用物理内存大小: GetProcessPhyMemSize
9.  驱动_获取进程命令行: GetProcessCmdline
10.  驱动_隐藏驱动: HideKernelModule

# 驱动2名称: Linux ARM64内核硬件断点进程调试驱动3
### 本驱动接口列表：
1.  驱动_打开进程: OpenProcess
2.  驱动_关闭进程: CloseHandle
3.  驱动_获取CPU支持硬件执行断点的数量: GetNumBRPS
4.  驱动_获取CPU支持硬件访问断点的数量: GetNumWRPS
5.  驱动_设置进程硬件断点: AddProcessHwBp
6.  驱动_删除进程硬件断点: DelProcessHwBp
7.  驱动_暂停硬件断点: SuspendProcessHwBp
8.  驱动_恢复硬件断点: ResumeProcessHwBp
9.  驱动_读取硬件断点命中信息: ReadHwBpInfo
10. 驱动_设置无条件Hook跳转: SetHookPC

## 目录说明：
> **rwProcMem33Module**：进程内存读写驱动
>>* **rwProcMem_module**：（*内核层*）驱动源码
>>* **testKo**：（*应用层*）调用驱动demo
>>* **testTarget**：（*应用层*）读取第三方进程demo
>>* **testMemSearch**：（*应用层*）搜索第三方进程内存demo
>>* **testDumpMem**：（*应用层*）保存第三方进程内存demo
>>* **testCEServer**：（*应用层*）CheatEngine远程服务器，可配合CheatEngine7.1远程连接使用

> **hwBreakpointProcModule**：硬件断点进程调试驱动
>>* **hwBreakpointProc_module**：（*内核层*）驱动源码
>>* **testHwBp**：（*应用层*）调用驱动demo
>>* **testHwBpTarget**：（*应用层*）硬件断点第三方进程demo
>>* **testHwBpClient**：（*应用层*）硬件断点工具之远程客户端
>>* **testHwBpServer**：（*应用层*）硬件断点工具之远程服务端

## 更新日志：
2025-7（读写驱动）：
  * **1.去除无用接口**
  * **2.修复驱动进程列表获取缺陷**
  * **3.修复驱动maps列表获取的一些bug**
  * **4.修复android15兼容性问题**
  * **5.新增一种驱动隐蔽通信手段**
  
2025-5（读写驱动）：
  * **1.支持Linux6.6**

2024-10（硬件断点驱动）：
  * **1.支持Linux5.10、6.1**
  * **2.修复硬件断点会卡死的bug**
  * **3.修复搜索Kit的一些bug**
