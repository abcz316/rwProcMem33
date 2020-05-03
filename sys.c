#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>

#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/version.h>

#include <linux/slab.h> 
#include <linux/device.h> 

#include "phy_mem.h"
#include "remote_proc_maps.h"
#include "remote_proc_cmdline.h"
#include "remote_proc_rss.h"
#include "version_control.h"


#define MAJOR_NUM 100

#define IOCTL_OPEN_PROCESS 								_IOR(MAJOR_NUM, 1, char*) 
#define IOCTL_CLOSE_HANDLE 								_IOR(MAJOR_NUM, 2, char*) 
#define IOCTL_GET_PROCESS_MAPS_COUNT			_IOR(MAJOR_NUM, 3, char*) 
#define IOCTL_GET_PROCESS_MAPS_LIST				_IOR(MAJOR_NUM, 4, char*) 
#define IOCTL_CHECK_PROCESS_ADDR_PHY			_IOR(MAJOR_NUM, 5, char*) 
#define IOCTL_GET_PROCESS_CMDLINE_ADDR		_IOR(MAJOR_NUM, 6, char*) 
#define IOCTL_GET_PROCESS_RSS							_IOR(MAJOR_NUM, 7, char*) 

static int g_rwProcMem_major  = 0; 
static dev_t g_rwProcMem_devno;


struct rwProcMemDev
{
	struct cdev cdev; 
	ssize_t mm_struct_arg_start_offset; 
};
struct rwProcMemDev *g_rwProcMem_devp; 
struct class *g_Class_devp; 

int rwProcMem_open(struct inode *inode, struct file *filp)
{
	filp->private_data = g_rwProcMem_devp;
	return 0; 
}

static ssize_t rwProcMem_read(struct file* filp, char __user* buf, size_t size, loff_t* ppos)
{
	char data[16];
	memset(data, 0, sizeof(data));

	if (copy_from_user(data, buf, 16))
	{
		return -EFAULT;
	}

	struct pid * proc_pid_struct = NULL;
	memcpy(&proc_pid_struct, data, sizeof(proc_pid_struct));
	printk_debug(KERN_INFO "read proc_pid_struct *:%ld\n", proc_pid_struct);

	
	size_t proc_virt_addr = 0; 
	memcpy(&proc_virt_addr, (void*)((size_t)data + (size_t)8), sizeof(proc_virt_addr));
	printk_debug(KERN_INFO "proc_virt_addr :0x%p\n", proc_virt_addr);


	if (!check_proc_map_can_read(proc_pid_struct, proc_virt_addr, size))
	{
		return -EFAULT;
	}


	
	if (clear_user(buf, size))
	{
		return -EFAULT;
	}

	size_t read_size = 0;
	while (read_size < size)
	{
		
		size_t phy_addr = get_proc_phy_addr(proc_pid_struct, proc_virt_addr + read_size);
		printk_debug(KERN_INFO "phy_addr:0x%p\n", phy_addr);
		if (phy_addr == 0)
		{
			break;
		}
		size_t pfn_sz = size_inside_page(phy_addr, ((size - read_size) > PAGE_SIZE) ? PAGE_SIZE : (size - read_size));
		printk_debug(KERN_INFO "pfn_sz:%ld\n", pfn_sz);
		read_ram_physical_addr_to_user(phy_addr, buf + read_size, pfn_sz);
		read_size += pfn_sz;
	}

	return read_size;
}

static ssize_t rwProcMem_write(struct file* filp, const char __user* buf, size_t size, loff_t *ppos)
{

	char data[16];
	memset(data, 0, sizeof(data));

	if (copy_from_user(data, buf, 16))
	{
		return -EFAULT;
	}
	
	struct pid * proc_pid_struct = NULL;
	memcpy(&proc_pid_struct, data, sizeof(proc_pid_struct));
	printk_debug(KERN_INFO "read proc_pid_struct *:%ld\n", proc_pid_struct);

	
	size_t proc_virt_addr = 0;
	memcpy(&proc_virt_addr, (void*)((size_t)data + (size_t)8), sizeof(proc_virt_addr));
	printk_debug(KERN_INFO "proc_virt_addr :0x%p\n", proc_virt_addr);


	if (!check_proc_map_can_write(proc_pid_struct, proc_virt_addr, size))
	{
		return -EFAULT;
	}

	size_t write_size = 0;
	while (write_size < size)
	{
		
		size_t phy_addr = get_proc_phy_addr(proc_pid_struct, proc_virt_addr + write_size);
		printk_debug(KERN_INFO "phy_addr:0x%p\n", phy_addr);
		if (phy_addr == 0)
		{
			break;
		}
		size_t pfn_sz = size_inside_page(phy_addr, ((size - write_size) > PAGE_SIZE) ? PAGE_SIZE : (size - write_size));
		printk_debug(KERN_INFO "pfn_sz:%ld\n", pfn_sz);
		write_ram_physical_addr_from_user(phy_addr, (void*)((size_t)buf + (size_t)16 + write_size), pfn_sz);
		write_size += pfn_sz;
	}
	return write_size;
}

static loff_t rwProcMem_llseek(struct file* filp, loff_t offset, int orig)
{
	loff_t ret = 0; 

	switch (orig)
	{
	case SEEK_SET: 
		if (offset < 0) 
		{
			ret = -EINVAL; 
			break;
		}

		filp->f_pos = offset; 
		ret = filp->f_pos; 
		break;

	case SEEK_CUR: 

		if ((filp->f_pos + offset) < 0) 
		{
			ret = -EINVAL;
			break;
		}

		filp->f_pos += offset;
		ret = filp->f_pos;
		break;

	default:
		ret = -EINVAL; 
		break;
	}

	return ret;
}




static long rwProcMem_ioctl(
	struct file *filp,
	unsigned int cmd,
	unsigned long arg)
{
	
	
	struct inode *inode = inode = filp->f_inode;



	struct rwProcMemDev* devp = filp->private_data; 
	
	switch (cmd)
	{
	case IOCTL_OPEN_PROCESS: 
	{

		printk_debug(KERN_INFO "IOCTL_OPEN_PROCESS\n");

		
		char buf[8];
		memset(&buf, 0, sizeof(buf));
		
		if (copy_from_user(buf, arg, 8))
		{
			return -EINVAL;
		}
		int64_t pid = 0;
		memcpy(&pid, buf, sizeof(pid));
		printk_debug(KERN_INFO "pid:%ld\n", pid);

		
		struct pid * proc_pid_struct = get_proc_pid_struct(pid);
		printk_debug(KERN_INFO "proc_pid_struct *:%ld\n", proc_pid_struct);

		if (!proc_pid_struct)
		{
			return -EINVAL;
		}
		
		memset(&buf, 0, sizeof(buf));
		memcpy(&buf, &proc_pid_struct, sizeof(proc_pid_struct));
		

		
		if (copy_to_user(arg, buf, 8))
		{
			return -EINVAL;
		}

		return 0;
		break;
	}
	case IOCTL_CLOSE_HANDLE: 
	{

		printk_debug(KERN_INFO "IOCTL_CLOSE_HANDLE\n");

		
		char buf[8];
		memset(&buf, 0, sizeof(buf));
		
		if (copy_from_user(buf, arg, 8))
		{
			return -EFAULT;
		}
		
		struct pid * proc_pid_struct = NULL;
		memcpy(&proc_pid_struct, buf, sizeof(proc_pid_struct));
		printk_debug(KERN_INFO "proc_pid_struct *:%ld\n", proc_pid_struct);
		release_proc_pid_struct(proc_pid_struct);


		return 0;
		break;
	}
	case IOCTL_GET_PROCESS_MAPS_COUNT: 
	{
	
		printk_debug(KERN_INFO "IOCTL_GET_PROCESS_MAPS_COUNT\n");

		char buf[8];
		memset(&buf, 0, 8);
		
		if (copy_from_user(buf, arg, 8))
		{
			return -EINVAL;
		}
		
		struct pid * proc_pid_struct = NULL;
		memcpy(&proc_pid_struct, buf, sizeof(proc_pid_struct));
		printk_debug(KERN_INFO "proc_pid_struct *:%ld\n", proc_pid_struct);

		return get_proc_map_count(proc_pid_struct);
		break;
	}
	case IOCTL_GET_PROCESS_MAPS_LIST: 
	{

		printk_debug(KERN_INFO "IOCTL_GET_PROCESS_MAPS_LIST\n");


		char buf[24];
		memset(&buf, 0, 24);
		
		if (copy_from_user(buf, arg, 24))
		{
			return -EINVAL;
		}
		
		struct pid * proc_pid_struct = NULL;
		memcpy(&proc_pid_struct, buf, sizeof(proc_pid_struct));
		printk_debug(KERN_INFO "proc_pid_struct *:%ld\n", proc_pid_struct);

		
		size_t name_len = 0;
		memcpy(&name_len, (void*)((size_t)buf + (size_t)8), sizeof(name_len));
		printk_debug(KERN_INFO "name_len:%ld\n", name_len);


		
		size_t buf_size = 0;
		memcpy(&buf_size, (void*)((size_t)buf + (size_t)16), sizeof(buf_size));
		printk_debug(KERN_INFO "buf_size:%ld\n", buf_size);

		int have_pass;
		int count = get_proc_maps_list_to_user(proc_pid_struct, name_len, (void*)((size_t)arg + (size_t)1), buf_size-1, &have_pass);
	
		
		unsigned char ch = have_pass == 1 ? '\x01' : '\x00';
		if (copy_to_user(arg, &ch, 1))
		{
			return -EFAULT;
		}
		return count;
		break;
	}
	case IOCTL_CHECK_PROCESS_ADDR_PHY: 
	{
	
		printk_debug(KERN_INFO "IOCTL_CHECK_PROC_ADDR_PHY\n");

		
		char buf[16];
		memset(&buf, 0, sizeof(buf));
		
		if (copy_from_user(buf, arg, 16))
		{
			return -EFAULT;
		}
		
		struct pid * proc_pid_struct = NULL;
		memcpy(&proc_pid_struct, buf, sizeof(proc_pid_struct));
		printk_debug(KERN_INFO "proc_pid_struct *:%ld\n", proc_pid_struct);

		
		size_t proc_virt_addr = 0;
		memcpy(&proc_virt_addr, (void*)((size_t)buf + (size_t)8), sizeof(proc_virt_addr));
		printk_debug(KERN_INFO "proc_virt_addr :0x%p\n", proc_virt_addr);

	
		if (get_proc_phy_addr(proc_pid_struct, proc_virt_addr))
		{
			return 1;
		}
		return 0;
		break;
	}
	case IOCTL_GET_PROCESS_CMDLINE_ADDR: 
	{

		printk_debug(KERN_INFO "IOCTL_GET_PROCESS_CMDLINE_ADDR\n");

		char buf[16];
		memset(&buf, 0, 16);
		
		if (copy_from_user(buf, arg, 16))
		{
			return -EINVAL;
		}
		
		struct pid * proc_pid_struct = NULL;
		memcpy(&proc_pid_struct, buf, sizeof(proc_pid_struct));
		printk_debug(KERN_INFO "proc_pid_struct *:%ld\n", proc_pid_struct);
		
		
		ssize_t relative_offset = 0;
		memcpy(&relative_offset, (void*)((size_t)buf + (size_t)8), sizeof(relative_offset));
		printk_debug(KERN_INFO "relative_offset:%ld\n", relative_offset);
		if (relative_offset!=-1)
		{
			devp->mm_struct_arg_start_offset = relative_offset;
		}

		memset(&buf, 0, 16);
		
		size_t arg_start = 0, arg_end = 0;
		int res = get_proc_cmdline_addr(proc_pid_struct, devp->mm_struct_arg_start_offset, &arg_start, &arg_end);
		memcpy((void*)buf, &arg_start, 8);
		memcpy((void*)((size_t)buf + (size_t)8), &arg_end, 8);
		
		if (copy_to_user(arg, &buf, 16)) 
		{
			return -EINVAL;
		}

		return res;
		break;
	}
	case IOCTL_GET_PROCESS_RSS: 
	{
	
		printk_debug(KERN_INFO "IOCTL_GET_PROCESS_RSS\n");

		char buf[8];
		memset(&buf, 0, 8);
		
		if (copy_from_user(buf, arg, 8))
		{
			return -EINVAL;
		}
		
		struct pid * proc_pid_struct = NULL;
		memcpy(&proc_pid_struct, buf, sizeof(proc_pid_struct));
		printk_debug(KERN_INFO "proc_pid_struct *:%ld\n", proc_pid_struct);

		
		memset(&buf, 0, 8);
		size_t rss = read_proc_rss_size(proc_pid_struct, devp->mm_struct_arg_start_offset);
		memcpy((void*)buf, &rss, 8);
		
		if (copy_to_user(arg, &buf, 8)) 
		{
			return -EINVAL;
		}
		return 0;
		break;
	}
	default:
		return -EINVAL;
	}

	return 0;
}

int rwProcMem_release(struct inode *inode, struct file *filp)
{
	return 0;
}


static const struct file_operations rwProcMem_fops =
{
  .owner = THIS_MODULE,
  .open = rwProcMem_open, 
  .release = rwProcMem_release, 
  .read = rwProcMem_read, 
  .write = rwProcMem_write, 
  .llseek = rwProcMem_llseek, 

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
	.ioctl = rwProcMem_ioctl, 
#else
	.compat_ioctl = rwProcMem_ioctl, 
	.unlocked_ioctl = rwProcMem_ioctl, 
#endif
};



static int rwProcMem_dev_init(void)
{
	int result;

	
	result = alloc_chrdev_region(&g_rwProcMem_devno, 0, 1, DEV_FILENAME);
	g_rwProcMem_major = MAJOR(g_rwProcMem_devno);


	if (result < 0)
	{
		printk(KERN_EMERG "rwProcMem alloc_chrdev_region failed %d\n", result);
		return result;
	}

	
	g_rwProcMem_devp = kmalloc(sizeof(struct rwProcMemDev), GFP_KERNEL);
	if (!g_rwProcMem_devp)
	{
		
		result = -ENOMEM;
		goto _fail;
	}
	memset(g_rwProcMem_devp, 0, sizeof(struct rwProcMemDev));

	
	int err;
	cdev_init(&g_rwProcMem_devp->cdev, &rwProcMem_fops); 
	g_rwProcMem_devp->cdev.owner = THIS_MODULE; 
	g_rwProcMem_devp->cdev.ops = &rwProcMem_fops; 
	
	err = cdev_add(&g_rwProcMem_devp->cdev, g_rwProcMem_devno, 1);
	if (err)
	{
		printk(KERN_NOTICE "Error in cdev_add()\n");
		result = -EFAULT;
		goto _fail;
	}


	
	g_Class_devp = class_create(THIS_MODULE, DEV_FILENAME); 
	device_create(g_Class_devp, NULL, g_rwProcMem_devno, NULL, "%s", DEV_FILENAME); 

#ifdef DEBUG_PRINTK
	printk(KERN_EMERG "Hello, rwProcMem %d debug\n", SYS_VERSION);
	
	
	
	
#else
	printk(KERN_EMERG "Hello, rwProcMem %d\n", SYS_VERSION);
#endif

	
	return 0;

_fail:
	unregister_chrdev_region(g_rwProcMem_devno, 1);
	return result;
}

static void rwProcMem_dev_exit(void)
{
	device_destroy(g_Class_devp, g_rwProcMem_devno); 
	class_destroy(g_Class_devp); 

	cdev_del(&g_rwProcMem_devp->cdev); 
	kfree(g_rwProcMem_devp); 
	unregister_chrdev_region(g_rwProcMem_devno, 1); 
	
	printk(KERN_EMERG "Goodbye, rwProcMem %d\n", SYS_VERSION);
}

module_init(rwProcMem_dev_init);
module_exit(rwProcMem_dev_exit);

MODULE_AUTHOR("abcz316");
MODULE_DESCRIPTION("Linux read & write process memory module. Coded by abcz316 2020-03-20");
MODULE_LICENSE("GPL");

