#ifndef SYS_H_
#define SYS_H_
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/version.h>
///////////////////////////////////////////////////////////////////
#include <linux/slab.h> //kmalloc与kfree
#include <linux/cdev.h>
#include <linux/device.h> //device_create创建设备文件（/dev/xxxxxxxx）
////////////////////////////////////////////////////////////////
#include "phy_mem.h"
#include "proc_maps.h"
#include "proc_list.h"
#include "proc_root.h"
#include "proc_rss.h"
#include "proc_cmdline.h"
#include "ver_control.h"
#include "test.h"

//////////////////////////////////////////////////////////////////

#define MAJOR_NUM 100

#define IOCTL_SET_MAX_DEV_FILE_OPEN				_IOWR(MAJOR_NUM, 0, char*) //设置驱动设备接口文件允许同时被使用的最大值
#define IOCTL_HIDE_KERNEL_MODULE						_IOWR(MAJOR_NUM, 1, char*) //隐藏驱动（卸载驱动需重启机器）
#define IOCTL_OPEN_PROCESS 								_IOWR(MAJOR_NUM, 2, char*) //打开进程
#define IOCTL_CLOSE_HANDLE 								_IOWR(MAJOR_NUM, 3, char*) //关闭进程
#define IOCTL_GET_PROCESS_MAPS_COUNT			_IOWR(MAJOR_NUM, 4, char*) //获取进程的内存块地址数量
#define IOCTL_GET_PROCESS_MAPS_LIST				_IOWR(MAJOR_NUM, 5, char*) //获取进程的内存块地址列表
#define IOCTL_CHECK_PROCESS_ADDR_PHY			_IOWR(MAJOR_NUM, 6, char*) //检查进程内存是否有物理内存位置
#define IOCTL_GET_PROCESS_PID_LIST					_IOWR(MAJOR_NUM, 7, char*) //获取进程PID列表
#define IOCTL_GET_PROCESS_GROUP						_IOWR(MAJOR_NUM, 8, char*) //获取进程权限等级
#define IOCTL_SET_PROCESS_ROOT							_IOWR(MAJOR_NUM, 9, char*) //提升进程权限到Root
#define IOCTL_GET_PROCESS_RSS							_IOWR(MAJOR_NUM, 10, char*) //获取进程的物理内存占用大小
#define IOCTL_GET_PROCESS_CMDLINE_ADDR		_IOWR(MAJOR_NUM, 11, char*) //获取进程cmdline的内存地址


//////////////////////////////////////////////////////////////////
MY_STATIC int g_rwProcMem_major = 0; //记录动态申请的主设备号
MY_STATIC dev_t g_rwProcMem_devno;

//rwProcMemDev设备结构体
struct rwProcMemDev {
	struct cdev cdev; //cdev结构体
	size_t max_dev_open_count; //驱动dev文件允许同时open的最大数量
	size_t cur_dev_open_count; //驱动dev文件当前同时open的数量
	bool is_already_hide_dev_file; //是否已经隐藏过驱动dev文件了
	bool is_already_hide_module_list; //是否已经隐藏过驱动列表了
};
MY_STATIC struct rwProcMemDev *g_rwProcMem_devp; //创建的cdev设备结构
MY_STATIC struct class *g_Class_devp; //创建的设备类

MY_STATIC int rwProcMem_open(struct inode *inode, struct file *filp);
MY_STATIC int rwProcMem_release(struct inode *inode, struct file *filp);
MY_STATIC ssize_t rwProcMem_read(struct file* filp, char __user* buf, size_t size, loff_t* ppos);
MY_STATIC ssize_t rwProcMem_write(struct file* filp, const char __user* buf, size_t size, loff_t *ppos);
//MY_STATIC long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
//MY_STATIC long (*compat_ioctl) (struct file *, unsigned int cmd, unsigned long arg);
MY_STATIC long rwProcMem_ioctl(
	struct file *filp,
	unsigned int cmd,
	unsigned long arg);
MY_STATIC loff_t rwProcMem_llseek(struct file* filp, loff_t offset, int orig);

MY_STATIC const struct file_operations rwProcMem_fops =
{
  .owner = THIS_MODULE,
  .open = rwProcMem_open, //打开设备函数
  .release = rwProcMem_release, //释放设备函数
  .read = rwProcMem_read, //读设备函数
  .write = rwProcMem_write, //写设备函数
  .llseek = rwProcMem_llseek, //定位偏移量函数

#if MY_LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
	.ioctl = rwProcMem_ioctl, //控制函数
#else
	.compat_ioctl = rwProcMem_ioctl, //64位驱动必须实现这个接口，32位程序才能调用驱动
	.unlocked_ioctl = rwProcMem_ioctl, //控制函数
#endif
};


#endif /* SYS_H_ */