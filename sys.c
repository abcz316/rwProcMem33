#include "sys.h"

MY_STATIC int rwProcMem_open(struct inode *inode, struct file *filp)
{
	//将设备结构体指针赋值给文件私有数据指针
	filp->private_data = g_rwProcMem_devp;

	g_rwProcMem_devp->cur_dev_open_count++;
	if (g_rwProcMem_devp->cur_dev_open_count >= g_rwProcMem_devp->max_dev_open_count)
	{
		if (!g_rwProcMem_devp->is_already_hide_dev_file)
		{
			g_rwProcMem_devp->is_already_hide_dev_file = true;
			//已达到驱动dev文件允许同时open的最大数量，可以删除设备文件，隐蔽性更强
			device_destroy(g_Class_devp, g_rwProcMem_devno); //删除设备文件（位置在/dev/xxxxx）
			class_destroy(g_Class_devp); //删除设备类
		}
		
	}
	return 0;
}


MY_STATIC ssize_t rwProcMem_read(struct file* filp, char __user* buf, size_t size, loff_t* ppos)
{
	
	//struct rwProcMemDev* devp = filp->private_data; //获得设备结构体指针
	

	char data[17] = {0};

	if (copy_from_user(data, buf, 17) == 0)
	{
		struct pid * proc_pid_struct = (struct pid *)*(size_t*)&data;
		size_t proc_virt_addr = *(size_t*)&data[8];
		bool is_force_read = data[16] == '\x01' ? true : false;

		size_t read_size = 0;

		printk_debug(KERN_INFO "READ proc_pid_struct*:0x%p,size:%ld\n", (void*)proc_pid_struct, sizeof(proc_pid_struct));

		printk_debug(KERN_INFO "proc_virt_addr:0x%zx,size:%ld\n", proc_virt_addr,sizeof(proc_virt_addr));


		if (is_force_read == false && !check_proc_map_can_read(proc_pid_struct, proc_virt_addr, size))
		{
			return -EFAULT;
		}

		//清空用户的缓冲区
		//if (clear_user(buf, size))
		//{
		//	return -EFAULT;
		//}


		while (read_size < size)
		{

			size_t phy_addr;
			size_t pfn_sz;
			size_t ret;
			char *lpOutBuf;


#ifdef CONFIG_USE_PAGEMAP_FILE
			struct file * pFile = open_pagemap(get_proc_pid(proc_pid_struct));
			printk_debug(KERN_INFO "open_pagemap %p\n", pFile);
			if (!pFile) { break; }

			phy_addr = get_pagemap_phy_addr(pFile, proc_virt_addr);

			close_pagemap(pFile);
#else
			pte_t *pte;
			bool old_pte_can_read;
			get_proc_phy_addr(&phy_addr, proc_pid_struct, proc_virt_addr + read_size, &pte);
#endif
			printk_debug(KERN_INFO "phy_addr:0x%zx\n", phy_addr);
			if (phy_addr == 0)
			{
				break;
			}

#ifdef CONFIG_USE_PAGEMAP_FILE
#else
			old_pte_can_read = is_pte_can_read(pte);

			if (is_force_read)
			{
		
				if (!old_pte_can_read)
				{
					if (!change_pte_read_status(pte, true)) { break; }

				}
			}

			else if (!old_pte_can_read) { break; }
#endif

			pfn_sz = size_inside_page(phy_addr, ((size - read_size) > PAGE_SIZE) ? PAGE_SIZE : (size - read_size));
			printk_debug(KERN_INFO "pfn_sz:%zu\n", pfn_sz);


			ret = 0;
			lpOutBuf = (char*)(buf + read_size);
			read_ram_physical_addr(&ret, phy_addr, lpOutBuf, false, pfn_sz);


#ifdef CONFIG_USE_PAGEMAP_FILE
#else
	
			if (is_force_read && old_pte_can_read == false)
			{
				change_pte_read_status(pte, false);
			}
#endif

			read_size += pfn_sz;
		}

		return read_size;
	}
	return -EFAULT;
}

MY_STATIC ssize_t rwProcMem_write(struct file* filp, const char __user* buf, size_t size, loff_t *ppos)
{

	//struct rwProcMemDev* devp = filp->private_data; //获得设备结构体指针


	char data[17] = {0};

	if (copy_from_user(data, buf, 17) == 0)
	{
		struct pid * proc_pid_struct = (struct pid *)*(size_t*)data;
		size_t proc_virt_addr = *(size_t*)&data[8];
		bool is_force_write = data[16] == '\x01' ? true : false;

		size_t write_size = 0;

		printk_debug(KERN_INFO "WRITE proc_pid_struct*:0x%p,size:%ld\n", (void*)proc_pid_struct,sizeof(proc_pid_struct));


		printk_debug(KERN_INFO "proc_virt_addr:0x%zx,size:%ld\n", proc_virt_addr,sizeof(proc_virt_addr));



		if (is_force_write == false && !check_proc_map_can_write(proc_pid_struct, proc_virt_addr, size))
		{
			return -EFAULT;
		}

		while (write_size < size)
		{

			size_t phy_addr;
			size_t pfn_sz;
			size_t ret;
			char *lpInputBuf;

#ifdef CONFIG_USE_PAGEMAP_FILE
			struct file * pFile = open_pagemap(get_proc_pid(proc_pid_struct));
			printk_debug(KERN_INFO "open_pagemap %d\n", pFile);
			if (!pFile) { break; }

			phy_addr = get_pagemap_phy_addr(pFile, proc_virt_addr);

			close_pagemap(pFile);
#else
			pte_t *pte;
			bool old_pte_can_write;
			get_proc_phy_addr(&phy_addr, proc_pid_struct, proc_virt_addr + write_size, &pte);
#endif

			printk_debug(KERN_INFO "phy_addr:0x%zx\n", phy_addr);
			if (phy_addr == 0)
			{
				break;
			}


#ifdef CONFIG_USE_PAGEMAP_FILE
#else
			old_pte_can_write = is_pte_can_write(pte);

			if (is_force_write)
			{
		
				if (!old_pte_can_write)
				{
					if (!change_pte_write_status(pte, true)) { break; }
				}
			}

			else if (!old_pte_can_write) { break; }
#endif

			pfn_sz = size_inside_page(phy_addr, ((size - write_size) > PAGE_SIZE) ? PAGE_SIZE : (size - write_size));
			printk_debug(KERN_INFO "pfn_sz:%zu\n", pfn_sz);



			ret = 0;
			lpInputBuf = (char*)(((size_t)buf + (size_t)17 + write_size));
			write_ram_physical_addr(&ret, phy_addr, lpInputBuf, false, pfn_sz);

#ifdef CONFIG_USE_PAGEMAP_FILE
#else
	
			if (is_force_write && old_pte_can_write == false)
			{
				change_pte_write_status(pte, false);
			}
#endif

			write_size += pfn_sz;
		}
		return write_size;
	}
	return -EFAULT;
}





//MY_STATIC long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
//MY_STATIC long (*compat_ioctl) (struct file *, unsigned int cmd, unsigned long arg);
MY_STATIC long rwProcMem_ioctl(
	struct file *filp,
	unsigned int cmd,
	unsigned long arg)
{
	//struct inode *inode = file_inode(filp);
	struct rwProcMemDev* devp = filp->private_data;
	switch (cmd)
	{
	case IOCTL_SET_MAX_DEV_FILE_OPEN:
	{
		char buf[8] = { 0 };
		if (copy_from_user((void*)buf, (void*)arg, 8) == 0)
		{
			size_t new_max_dev_open_count = *(size_t*)buf;

			printk_debug(KERN_INFO "IOCTL_SET_MAX_DEV_FILE_OPEN\n");

			printk_debug(KERN_INFO "new_max_dev_open_count:%zu,size:%ld\n", new_max_dev_open_count, sizeof(new_max_dev_open_count));

			devp->max_dev_open_count = new_max_dev_open_count;



			if (g_rwProcMem_devp->cur_dev_open_count >= g_rwProcMem_devp->max_dev_open_count)
			{
				if (!g_rwProcMem_devp->is_already_hide_dev_file)
				{
					g_rwProcMem_devp->is_already_hide_dev_file = true;
					
					device_destroy(g_Class_devp, g_rwProcMem_devno); 
					class_destroy(g_Class_devp);
				}

			}
			else if (g_rwProcMem_devp->cur_dev_open_count < g_rwProcMem_devp->max_dev_open_count)
			{
				if (g_rwProcMem_devp->is_already_hide_dev_file)
				{
					g_rwProcMem_devp->is_already_hide_dev_file = false;
					
					g_Class_devp = class_create(THIS_MODULE, DEV_FILENAME);
					device_create(g_Class_devp, NULL, g_rwProcMem_devno, NULL, "%s", DEV_FILENAME);
				}

			}
			return 0;
		}
		return -EINVAL;
		
		break;
	}
	case IOCTL_HIDE_KERNEL_MODULE:
	{

		printk_debug(KERN_INFO "IOCTL_HIDE_KERNEL_MODULE\n");

		if (g_rwProcMem_devp->is_already_hide_module_list == false)
		{
			g_rwProcMem_devp->is_already_hide_module_list = true; 

			list_del_init(&__this_module.list);

			kobject_del(&THIS_MODULE->mkobj.kobj);
		}
		return 0;
		
		break;
	}
	case IOCTL_OPEN_PROCESS: 
	{


		char buf[8] = { 0 };

		if (copy_from_user((void*)buf, (void*)arg, 8) == 0)
		{
			uint64_t pid = *(uint64_t*)&buf;
			struct pid * proc_pid_struct = NULL;

			printk_debug(KERN_INFO "IOCTL_OPEN_PROCESS\n");

			printk_debug(KERN_INFO "pid:%llu,size:%ld\n", pid,sizeof(pid));


			proc_pid_struct = get_proc_pid_struct(pid);
			printk_debug(KERN_INFO "proc_pid_struct *:0x%p\n", (void*)proc_pid_struct);

			if (!proc_pid_struct)
			{
				return -EINVAL;
			}

			memset(&buf, 0, sizeof(buf));
			memcpy(&buf, &proc_pid_struct, sizeof(proc_pid_struct));

			if (!!copy_to_user((void*)arg, (void*)buf, 8))
			{
				return -EINVAL;
			}

			return 0;

		}

		return -EINVAL;

		break;
	}
	case IOCTL_CLOSE_HANDLE: 
	{

		char buf[8] = { 0 };

		if (copy_from_user((void*)buf, (void*)arg, 8) == 0)
		{
			struct pid * proc_pid_struct = (struct pid *)*(size_t*)buf;
			printk_debug(KERN_INFO "IOCTL_CLOSE_HANDLE\n");


			printk_debug(KERN_INFO "proc_pid_struct*:0x%p,size:%ld\n", (void*)proc_pid_struct,sizeof(proc_pid_struct));

	
			release_proc_pid_struct(proc_pid_struct);

			return 0;

		}
		return -EFAULT;
		break;
	}
	case IOCTL_GET_PROCESS_MAPS_COUNT:
	{


		char buf[8] = { 0 };

		if (copy_from_user((void*)buf, (void*)arg, 8) == 0)
		{
			struct pid * proc_pid_struct = (struct pid *)*(size_t*)buf;

			printk_debug(KERN_INFO "IOCTL_GET_PROCESS_MAPS_COUNT\n");


			printk_debug(KERN_INFO "proc_pid_struct*:0x%p,size:%ld\n", (void*)proc_pid_struct, sizeof(proc_pid_struct));

			return get_proc_map_count(proc_pid_struct);
		}
		return -EINVAL;
		break;
	}
	case IOCTL_GET_PROCESS_MAPS_LIST: 
	{
	
		char buf[24];

		if (copy_from_user((void*)buf, (void*)arg, 24) == 0)
		{
			struct pid * proc_pid_struct = (struct pid *)*(size_t*)buf;
			size_t name_len = *(size_t*)&buf[8];
			size_t buf_size = *(size_t*)&buf[16];
			int have_pass = 0;
			int count = 0;
			unsigned char ch;
			printk_debug(KERN_INFO "IOCTL_GET_PROCESS_MAPS_LIST\n");


			printk_debug(KERN_INFO "proc_pid_struct*:0x%p,size:%ld\n", (void*)proc_pid_struct, sizeof(proc_pid_struct));


			printk_debug(KERN_INFO "name_len:%zu,size:%ld\n", name_len, sizeof(name_len));



			printk_debug(KERN_INFO "buf_size:%zu,size:%ld\n", buf_size, sizeof(buf_size));

			count = get_proc_maps_list(proc_pid_struct, name_len, (void*)((size_t)arg + (size_t)1), buf_size - 1, false, &have_pass);


			ch = have_pass == 1 ? '\x01' : '\x00';
			if (!!copy_to_user((void*)arg, &ch, 1))
			{
				return -EFAULT;
			}
			return count;


		}

		return -EINVAL;

		
		break;
	}
	case IOCTL_CHECK_PROCESS_ADDR_PHY:
	{

		char buf[16] = { 0 };
	
		if (copy_from_user((void*)buf, (void*)arg, 16) == 0)
		{
			struct pid * proc_pid_struct = (struct pid *)*(size_t*)buf;
			size_t proc_virt_addr = *(size_t*)&buf[8];
#ifdef CONFIG_USE_PAGEMAP_FILE
			struct file * pFile;
#else
			pte_t *pte;
#endif
			size_t ret;

			printk_debug(KERN_INFO "IOCTL_CHECK_PROC_ADDR_PHY\n");


			printk_debug(KERN_INFO "proc_pid_struct *:0x%p,size:%ld\n", (void*)proc_pid_struct, sizeof(proc_pid_struct));


			printk_debug(KERN_INFO "proc_virt_addr :0x%zx,size:%ld\n", proc_virt_addr, sizeof(proc_virt_addr));


#ifdef CONFIG_USE_PAGEMAP_FILE
			pFile = open_pagemap(get_proc_pid(proc_pid_struct));
			printk_debug(KERN_INFO "open_pagemap %d\n", pFile);
			if (!pFile) { break; }
			ret = get_pagemap_phy_addr(pFile, proc_virt_addr);
			close_pagemap(pFile);
#else
			get_proc_phy_addr(&ret, proc_pid_struct, proc_virt_addr, &pte);
#endif

			if (ret)
			{
				return 1;
			}
			return 0;
		}
		return -EFAULT;
		
		break;
	}


	case IOCTL_GET_PROCESS_PID_LIST: 
	{
	
		char buf[9] = { 0 };

		if (copy_from_user((void*)buf, (void*)arg, 9) == 0)
		{
			uint64_t buf_size = *(uint64_t*)&buf;
			bool is_speed_mode = buf[8] == '\x01' ? true : false;
			ssize_t proc_count = 0;
			printk_debug(KERN_INFO "IOCTL_GET_PROCESS_PID_LIST\n");

			printk_debug(KERN_INFO "buf_size:%llu,size:%ld\n", buf_size, sizeof(buf_size));

	
			printk_debug(KERN_INFO "is_speed_mode:%d,size:%ld\n", is_speed_mode, sizeof(is_speed_mode));

			proc_count = get_proc_pid_list(!is_speed_mode, (char*)arg, buf_size, false);
			return proc_count;
		}
		return -EFAULT;
		break;
	}
	case IOCTL_GET_PROCESS_GROUP:
	{

		char buf[64] = { 0 };

		if (copy_from_user((void*)buf, (void*)arg, 8) == 0)
		{
			int res = 0;
			struct pid * proc_pid_struct = (struct pid *)*(size_t*)buf;
			size_t nOutUID = 0;
			size_t nOutSUID = 0;
			size_t nOutEUID = 0;
			size_t nOutFSUID = 0;
			size_t nOutGID = 0;
			size_t nOutSGID = 0;
			size_t nOutEGID = 0;
			size_t nOutFSGID = 0;

			printk_debug(KERN_INFO "IOCTL_GET_PROCESS_GROUP\n");


			printk_debug(KERN_INFO "proc_pid_struct*:0x%p,size:%ld\n", (void*)proc_pid_struct, sizeof(proc_pid_struct));

			memset(buf, 0, 8);

			res = get_proc_group(
				proc_pid_struct,
				&nOutUID,
				&nOutSUID,
				&nOutEUID,
				&nOutFSUID,
				&nOutGID,
				&nOutSGID,
				&nOutEGID,
				&nOutFSGID);

			*(size_t*)&buf[0] = nOutUID;
			*(size_t*)&buf[8] = nOutSUID;
			*(size_t*)&buf[16] = nOutEUID;
			*(size_t*)&buf[24] = nOutFSUID;
			*(size_t*)&buf[32] = nOutGID;
			*(size_t*)&buf[40] = nOutSGID;
			*(size_t*)&buf[48] = nOutEGID;
			*(size_t*)&buf[56] = nOutFSGID;



			if (!!copy_to_user((void*)arg, &buf, sizeof(buf)))
			{
				return -EINVAL;
			}
			return res;
		}
		return -EFAULT;
		break;
	}
	case IOCTL_SET_PROCESS_ROOT: 
	{

		char buf[8] = { 0 };
	
		if (copy_from_user((void*)buf, (void*)arg, 8) == 0)
		{
			struct pid * proc_pid_struct = (struct pid *)*(size_t*)buf;
			printk_debug(KERN_INFO "IOCTL_SET_PROCESS_ROOT\n");


			printk_debug(KERN_INFO "proc_pid_struct*:0x%p,size:%ld\n", (void*)proc_pid_struct, sizeof(proc_pid_struct));

			return set_proc_root(proc_pid_struct);
		}
		return -EFAULT;
		break;
	}
	case IOCTL_GET_PROCESS_RSS: 
	{

		char buf[8] = { 0 };

		if (copy_from_user((void*)buf, (void*)arg, 8) == 0)
		{
			struct pid * proc_pid_struct = (struct pid *)*(size_t*)buf;
			size_t rss;
			printk_debug(KERN_INFO "IOCTL_GET_PROCESS_RSS\n");


			printk_debug(KERN_INFO "proc_pid_struct*:0x%p,size:%ld\n", (void*)proc_pid_struct, sizeof(proc_pid_struct));

	
			memset(&buf, 0, 8);
			rss = read_proc_rss_size(proc_pid_struct);
			memcpy((void*)buf, &rss, 8);

			if (!!copy_to_user((void*)arg, &buf, 8)) //输出
			{
				return -EINVAL;
			}
			return 0;
		}
		return -EINVAL;

		break;
	}
	case IOCTL_GET_PROCESS_CMDLINE_ADDR: 
	{

		char buf[16] = { 0 };

		if (copy_from_user((void*)buf, (void*)arg, 8) == 0)
		{
			struct pid * proc_pid_struct = (struct pid *)*(size_t*)buf;
			size_t arg_start = 0, arg_end = 0;
			int res;

			printk_debug(KERN_INFO "IOCTL_GET_PROCESS_CMDLINE_ADDR\n");


			printk_debug(KERN_INFO "proc_pid_struct *:0x%p,size:%ld\n", (void*)proc_pid_struct, sizeof(proc_pid_struct));

			memset(&buf, 0, 16);


			res = get_proc_cmdline_addr(proc_pid_struct, &arg_start, &arg_end);

			memcpy((void*)buf, &arg_start, 8);
			memcpy((void*)((size_t)buf + (size_t)8), &arg_end, 8);

			if (!!copy_to_user((void*)arg, &buf, 16))
			{
				return -EINVAL;
			}

			return res;
		}

		return -EINVAL;


		break;
	}
	default:
		return -EINVAL;
	}
	return -EINVAL;

}
MY_STATIC loff_t rwProcMem_llseek(struct file* filp, loff_t offset, int orig)
{
	loff_t ret = 0; //返回的位置偏移

	switch (orig)
	{
	case SEEK_SET: //相对文件开始位置偏移
		if (offset < 0) //offset不合法
		{
			ret = -EINVAL; //无效的指针
			break;
		}

		filp->f_pos = offset; //更新文件指针位置
		ret = filp->f_pos; //返回的位置偏移
		break;

	case SEEK_CUR: //相对文件当前位置偏移

		if ((filp->f_pos + offset) < 0) //指针不合法
		{
			ret = -EINVAL;//无效的指针
			break;
		}

		filp->f_pos += offset;
		ret = filp->f_pos;
		break;

	default:
		ret = -EINVAL; //无效的指针
		break;
	}

	return ret;
}

MY_STATIC int rwProcMem_release(struct inode *inode, struct file *filp)
{

	g_rwProcMem_devp->cur_dev_open_count--;
	if (g_rwProcMem_devp->cur_dev_open_count < g_rwProcMem_devp->max_dev_open_count)
	{
		if (g_rwProcMem_devp->is_already_hide_dev_file)
		{
			g_rwProcMem_devp->is_already_hide_dev_file = false;
			//驱动连接被关闭时，重新创建设备文件（位置在/dev/xxxxx）
			g_Class_devp = class_create(THIS_MODULE, DEV_FILENAME); //创建设备类
			device_create(g_Class_devp, NULL, g_rwProcMem_devno, NULL, "%s", DEV_FILENAME); //创建设备文件
		}

	}
	return 0;
}

#ifndef CONFIG_MODULE_GUIDE_ENTRY
static
#endif
int __init rwProcMem_dev_init(void)
{
	int result;
	int err;
	//动态申请设备号
	result = alloc_chrdev_region(&g_rwProcMem_devno, 0, 1, DEV_FILENAME);
	g_rwProcMem_major = MAJOR(g_rwProcMem_devno);

	if (result < 0)
	{
		printk(KERN_EMERG "rwProcMem alloc_chrdev_region failed %d\n", result);
		return result;
	}

	// 2.动态申请设备结构体的内存
	g_rwProcMem_devp = kmalloc(sizeof(struct rwProcMemDev), GFP_KERNEL);
	if (!g_rwProcMem_devp)
	{
		//申请失败
		result = -ENOMEM;
		goto _fail;
	}
	memset(g_rwProcMem_devp, 0, sizeof(struct rwProcMemDev));

	//3.初始化并且添加cdev结构体
	cdev_init(&g_rwProcMem_devp->cdev, &rwProcMem_fops); //初始化cdev设备
	g_rwProcMem_devp->cdev.owner = THIS_MODULE; //使驱动程序属于该模块
	g_rwProcMem_devp->cdev.ops = &rwProcMem_fops; //cdev连接file_operations指针
	//将cdev注册到系统中
	err = cdev_add(&g_rwProcMem_devp->cdev, g_rwProcMem_devno, 1);
	if (err)
	{
		printk(KERN_NOTICE "Error in cdev_add()\n");
		result = -EFAULT;
		goto _fail;
	}
	g_rwProcMem_devp->max_dev_open_count = 1; //驱动dev文件允许同时open的最大数量
	g_rwProcMem_devp->is_already_hide_dev_file = false; //是否已经隐藏过驱动dev文件了：否
	g_rwProcMem_devp->is_already_hide_module_list = false; //是否已经隐藏过驱动列表了：否

	//4.创建设备文件
	g_Class_devp = class_create(THIS_MODULE, DEV_FILENAME); //创建设备类（位置在/sys/class/xxxxx）
	device_create(g_Class_devp, NULL, g_rwProcMem_devno, NULL, "%s", DEV_FILENAME); //创建设备文件（位置在/dev/xxxxx）




#ifdef CONFIG_DEBUG_PRINTK
	printk(KERN_EMERG "Hello, %s debug\n", DEV_FILENAME);
	//test1();
	//test2();
	//test3();
	//test4();
	//test5();
#else
	printk(KERN_EMERG "Hello, %s\n", DEV_FILENAME);
#endif
	return 0;
_fail:
	unregister_chrdev_region(g_rwProcMem_devno, 1);
	return result;
}

#ifndef CONFIG_MODULE_GUIDE_ENTRY
static
#endif
void __exit rwProcMem_dev_exit(void)
{
	device_destroy(g_Class_devp, g_rwProcMem_devno); //删除设备文件（位置在/dev/xxxxx）
	class_destroy(g_Class_devp); //删除设备类

	cdev_del(&g_rwProcMem_devp->cdev); //注销cdev
	kfree(g_rwProcMem_devp); // 释放设备结构体内存
	unregister_chrdev_region(g_rwProcMem_devno, 1); //释放设备号

	printk(KERN_EMERG "Goodbye, %s\n", DEV_FILENAME);

}

#ifdef CONFIG_MODULE_GUIDE_ENTRY
module_init(rwProcMem_dev_init);
module_exit(rwProcMem_dev_exit);
#endif

MODULE_AUTHOR("Linux");
MODULE_DESCRIPTION("Linux default module");
MODULE_LICENSE("GPL");

