#include "sys.h"

MY_STATIC int rwProcMem_open(struct inode *inode, struct file *filp) {
	printk_debug(KERN_INFO "rwProcMem_open!!!!\n");

	g_rwProcMem_devp->cur_dev_open_count++;
	if (g_rwProcMem_devp->cur_dev_open_count >= g_rwProcMem_devp->max_dev_open_count) {
		if (!g_rwProcMem_devp->is_already_hide_dev_file) {
			g_rwProcMem_devp->is_already_hide_dev_file = true;

			//#ifdef CONFIG_USE_PROC_FILE_NODE
			//			//proc_remove(g_rwProcMem_devp->proc_entry);
			//#endif

#ifdef CONFIG_USE_DEV_FILE_NODE
			device_destroy(g_Class_devp, g_rwProcMem_devno);
			class_destroy(g_Class_devp);
#endif
		}

	}
	return 0;
}

MY_STATIC int rwProcMem_release(struct inode *inode, struct file *filp) {
	printk_debug(KERN_INFO "rwProcMem_release!!!!\n");
	if (g_rwProcMem_devp->cur_dev_open_count > 0) {
		g_rwProcMem_devp->cur_dev_open_count--;
	}
	if (g_rwProcMem_devp->cur_dev_open_count < g_rwProcMem_devp->max_dev_open_count) {
		if (g_rwProcMem_devp->is_already_hide_dev_file) {
			g_rwProcMem_devp->is_already_hide_dev_file = false;

			//#ifdef CONFIG_USE_PROC_FILE_NODE
			//			g_rwProcMem_devp->proc_entry = proc_create(DEV_FILENAME, S_IRUGO | S_IWUGO, NULL, &rwProcMem_fops);
			//#endif
#ifdef CONFIG_USE_DEV_FILE_NODE
			g_Class_devp = class_create(THIS_MODULE, DEV_FILENAME);
			device_create(g_Class_devp, NULL, g_rwProcMem_devno, NULL, "%s", DEV_FILENAME);
#endif

		}

	}
	return 0;
}


MY_STATIC ssize_t rwProcMem_read(struct file* filp, char __user* buf, size_t size, loff_t* ppos) {
	char data[17] = { 0 };
	unsigned long read = x_copy_from_user(data, buf, 17);
	if (read == 0) {
		struct pid * proc_pid_struct = (struct pid *)*(size_t*)&data;
		size_t proc_virt_addr = *(size_t*)&data[8];
		bool is_force_read = data[16] == '\x01' ? true : false;

		size_t read_size = 0;

		printk_debug(KERN_INFO "READ proc_pid_struct*:0x%p,size:%ld\n", (void*)proc_pid_struct, sizeof(proc_pid_struct));
		printk_debug(KERN_INFO "READ proc_virt_addr:0x%zx,size:%ld\n", proc_virt_addr, sizeof(proc_virt_addr));


		if (is_force_read == false && !check_proc_map_can_read(proc_pid_struct, proc_virt_addr, size)) {
			return -EFAULT;
		}

		//if (clear_user(buf, size))
		//{
		//	return -EFAULT;
		//}


		while (read_size < size) {
			size_t phy_addr = 0;
			size_t pfn_sz = 0;
			char *lpOutBuf = NULL;


#ifdef CONFIG_USE_PAGEMAP_FILE_CALC_PHY_ADDR
			struct file * pFile = open_pagemap(get_proc_pid(proc_pid_struct));
			printk_debug(KERN_INFO "open_pagemap %d\n", pFile);
			if (!pFile) { break; }

			phy_addr = get_pagemap_phy_addr(pFile, proc_virt_addr);

			close_pagemap(pFile);
			printk_debug(KERN_INFO "pagemap phy_addr:0x%zx\n", phy_addr);
#endif

#ifdef CONFIG_USE_PAGE_TABLE_CALC_PHY_ADDR
			pte_t *pte;

			bool old_pte_can_read;
			phy_addr = get_proc_phy_addr(proc_pid_struct, proc_virt_addr + read_size, (pte_t*)&pte);
			printk_debug(KERN_INFO "calc phy_addr:0x%zx\n", phy_addr);

#endif
			if (phy_addr == 0) {
				break;
			}

#ifdef CONFIG_USE_PAGE_TABLE_CALC_PHY_ADDR
			old_pte_can_read = is_pte_can_read(pte);
			if (is_force_read) {
				if (!old_pte_can_read) {
					if (!change_pte_read_status(pte, true)) { break; }

				}
			}
			else if (!old_pte_can_read) { break; }
#endif

			pfn_sz = size_inside_page(phy_addr, ((size - read_size) > PAGE_SIZE) ? PAGE_SIZE : (size - read_size));
			printk_debug(KERN_INFO "pfn_sz:%zu\n", pfn_sz);


			lpOutBuf = (char*)(buf + read_size);
			read_ram_physical_addr(phy_addr, lpOutBuf, false, pfn_sz);


#ifdef CONFIG_USE_PAGE_TABLE_CALC_PHY_ADDR
			if (is_force_read && old_pte_can_read == false) {
				change_pte_read_status(pte, false);
			}
#endif

			read_size += pfn_sz;
		}


		return read_size;
	} else {
		printk_debug(KERN_INFO "READ FAILED ret:%lu, user:%p, size:%zu\n", read, buf, size);

	}
	return -EFAULT;
}

MY_STATIC ssize_t rwProcMem_write(struct file* filp, const char __user* buf, size_t size, loff_t *ppos) {
	char data[17] = { 0 };
	unsigned long write = x_copy_from_user(data, buf, 17);
	if (write == 0) {
		struct pid * proc_pid_struct = (struct pid *)*(size_t*)data;
		size_t proc_virt_addr = *(size_t*)&data[8];
		bool is_force_write = data[16] == '\x01' ? true : false;

		size_t write_size = 0;
		printk_debug(KERN_INFO "WRITE proc_pid_struct*:0x%p,size:%ld\n", (void*)proc_pid_struct, sizeof(proc_pid_struct));
		printk_debug(KERN_INFO "WRITE proc_virt_addr:0x%zx,size:%ld\n", proc_virt_addr, sizeof(proc_virt_addr));

		if (is_force_write == false && !check_proc_map_can_write(proc_pid_struct, proc_virt_addr, size)) {
			return -EFAULT;
		}

		while (write_size < size) {
			size_t phy_addr = 0;
			size_t pfn_sz = 0;
			char *lpInputBuf = NULL;

#ifdef CONFIG_USE_PAGEMAP_FILE_CALC_PHY_ADDR
			struct file * pFile = open_pagemap(get_proc_pid(proc_pid_struct));
			printk_debug(KERN_INFO "open_pagemap %d\n", pFile);
			if (!pFile) { break; }

			phy_addr = get_pagemap_phy_addr(pFile, proc_virt_addr);

			close_pagemap(pFile);
#endif

#ifdef CONFIG_USE_PAGE_TABLE_CALC_PHY_ADDR
			pte_t *pte;
			bool old_pte_can_write;
			phy_addr = get_proc_phy_addr(proc_pid_struct, proc_virt_addr + write_size, (pte_t*)&pte);
#endif

			printk_debug(KERN_INFO "phy_addr:0x%zx\n", phy_addr);
			if (phy_addr == 0) {
				break;
			}


#ifdef CONFIG_USE_PAGE_TABLE_CALC_PHY_ADDR
			old_pte_can_write = is_pte_can_write(pte);
			if (is_force_write) {
				if (!old_pte_can_write) {
					if (!change_pte_write_status(pte, true)) { break; }
				}
			}
			else if (!old_pte_can_write) { break; }
#endif

			pfn_sz = size_inside_page(phy_addr, ((size - write_size) > PAGE_SIZE) ? PAGE_SIZE : (size - write_size));
			printk_debug(KERN_INFO "pfn_sz:%zu\n", pfn_sz);



			lpInputBuf = (char*)(((size_t)buf + (size_t)17 + write_size));
			write_ram_physical_addr(phy_addr, lpInputBuf, false, pfn_sz);

#ifdef CONFIG_USE_PAGE_TABLE_CALC_PHY_ADDR
			if (is_force_write && old_pte_can_write == false) {
				change_pte_write_status(pte, false);
			}
#endif

			write_size += pfn_sz;
		}
		return write_size;
	} else {
		printk_debug(KERN_INFO "WRITE FAILED ret:%lu, user:%p, size:%zu\n", write, buf, size);
	}
	return -EFAULT;
}



MY_STATIC inline long DispatchCommand(unsigned int cmd, unsigned long arg) {

	switch (cmd) {
	case IOCTL_INIT_DEVICE_INFO:
	{
		size_t size = 0;
		struct init_device_info* p_init_device_info = (struct init_device_info*)kmalloc(sizeof(struct init_device_info), GFP_KERNEL);
		if (!p_init_device_info) {
			return -ENOMEM;
			break;
		}
		memset(p_init_device_info, 0, sizeof(struct init_device_info));

		size = copy_from_user((void*)p_init_device_info, (void*)arg, sizeof(struct init_device_info));
		if (size == 0) {
			printk_debug(KERN_INFO "IOCTL_INIT_DEVICE_INFO\n");
			init_mmap_lock_offset(p_init_device_info->proc_self_maps_cnt);
			init_map_count_offset(p_init_device_info->proc_self_maps_cnt);
			init_proc_cmdline_offset(p_init_device_info->proc_self_cmdline, get_task_proc_cmdline_addr);
			init_proc_root_offset(p_init_device_info->proc_self_status);

		}
		kfree(p_init_device_info);

		return size == 0  ? 0 : -EINVAL;
		break;
	}
	case IOCTL_SET_MAX_DEV_FILE_OPEN:
	{
		char buf[8] = { 0 };
		if (x_copy_from_user((void*)buf, (void*)arg, 8) == 0) {
			size_t new_max_dev_open_count = *(size_t*)buf;

			printk_debug(KERN_INFO "IOCTL_SET_MAX_DEV_FILE_OPEN\n");

			printk_debug(KERN_INFO "new_max_dev_open_count:%zu,size:%ld\n", new_max_dev_open_count, sizeof(new_max_dev_open_count));

			g_rwProcMem_devp->max_dev_open_count = new_max_dev_open_count;



			if (g_rwProcMem_devp->cur_dev_open_count >= g_rwProcMem_devp->max_dev_open_count) {
				if (!g_rwProcMem_devp->is_already_hide_dev_file) {
					g_rwProcMem_devp->is_already_hide_dev_file = true;

#ifdef CONFIG_USE_PROC_FILE_NODE
					proc_remove(g_rwProcMem_devp->proc_entry);
#endif
#ifdef CONFIG_USE_DEV_FILE_NODE
					device_destroy(g_Class_devp, g_rwProcMem_devno);
					class_destroy(g_Class_devp);
#endif
				}

			} else if (g_rwProcMem_devp->cur_dev_open_count < g_rwProcMem_devp->max_dev_open_count) {
				if (g_rwProcMem_devp->is_already_hide_dev_file) {
					g_rwProcMem_devp->is_already_hide_dev_file = false;

#ifdef CONFIG_USE_PROC_FILE_NODE
					g_rwProcMem_devp->proc_entry = proc_create(DEV_FILENAME, S_IRUGO | S_IWUGO, NULL, &rwProcMem_fops);
#endif
#ifdef CONFIG_USE_DEV_FILE_NODE
					g_Class_devp = class_create(THIS_MODULE, DEV_FILENAME);
					device_create(g_Class_devp, NULL, g_rwProcMem_devno, NULL, "%s", DEV_FILENAME); //创建设备文件
#endif
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

		if (g_rwProcMem_devp->is_already_hide_module_list == false) {
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
		if (x_copy_from_user((void*)buf, (void*)arg, 8) == 0)
		{
			uint64_t pid = *(uint64_t*)&buf;
			struct pid * proc_pid_struct = NULL;

			printk_debug(KERN_INFO "IOCTL_OPEN_PROCESS\n");

			printk_debug(KERN_INFO "pid:%llu,size:%ld\n", pid, sizeof(pid));

			proc_pid_struct = get_proc_pid_struct(pid);
			printk_debug(KERN_INFO "proc_pid_struct *:0x%p\n", (void*)proc_pid_struct);

			if (!proc_pid_struct) {
				return -EINVAL;
			}

			//memcpy(&buf, &proc_pid_struct, sizeof(proc_pid_struct));
			*(size_t *)&buf[0] = (size_t)proc_pid_struct;

			if (!!x_copy_to_user((void*)arg, (void*)buf, 8))
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
		if (x_copy_from_user((void*)buf, (void*)arg, 8) == 0) {
			struct pid * proc_pid_struct = (struct pid *)*(size_t*)buf;
			printk_debug(KERN_INFO "IOCTL_CLOSE_HANDLE\n");

			printk_debug(KERN_INFO "proc_pid_struct*:0x%p,size:%ld\n", (void*)proc_pid_struct, sizeof(proc_pid_struct));

			release_proc_pid_struct(proc_pid_struct);

			return 0;

		}
		return -EFAULT;
		break;
	}
	case IOCTL_GET_PROCESS_MAPS_COUNT:
	{
		char buf[8] = { 0 };
		if (x_copy_from_user((void*)buf, (void*)arg, 8) == 0) {
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
		if (x_copy_from_user((void*)buf, (void*)arg, 24) == 0) {
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
			if (!!x_copy_to_user((void*)arg, &ch, 1)) {
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
		if (x_copy_from_user((void*)buf, (void*)arg, 16) == 0) {
			struct pid * proc_pid_struct = (struct pid *)*(size_t*)buf;
			size_t proc_virt_addr = *(size_t*)&buf[8];
#ifdef CONFIG_USE_PAGEMAP_FILE_CALC_PHY_ADDR
			struct file * pFile;
#endif

#ifdef CONFIG_USE_PAGE_TABLE_CALC_PHY_ADDR
			pte_t *pte;
#endif
			size_t ret = 0;

			printk_debug(KERN_INFO "IOCTL_CHECK_PROC_ADDR_PHY\n");
			printk_debug(KERN_INFO "proc_pid_struct *:0x%p,size:%ld\n", (void*)proc_pid_struct, sizeof(proc_pid_struct));
			printk_debug(KERN_INFO "proc_virt_addr :0x%zx,size:%ld\n", proc_virt_addr, sizeof(proc_virt_addr));

#ifdef CONFIG_USE_PAGEMAP_FILE_CALC_PHY_ADDR
			pFile = open_pagemap(get_proc_pid(proc_pid_struct));
			printk_debug(KERN_INFO "open_pagemap %p\n", pFile);
			if (!pFile) { break; }
			ret = get_pagemap_phy_addr(pFile, proc_virt_addr);
			close_pagemap(pFile);
#endif

#ifdef CONFIG_USE_PAGE_TABLE_CALC_PHY_ADDR
			ret = get_proc_phy_addr(proc_pid_struct, proc_virt_addr, (pte_t*)&pte);
#endif

			if (ret) {
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
		if (x_copy_from_user((void*)buf, (void*)arg, 9) == 0) {
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
		if (x_copy_from_user((void*)buf, (void*)arg, 8) == 0) {
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

			if (!!x_copy_to_user((void*)arg, &buf, sizeof(buf)))
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
		if (x_copy_from_user((void*)buf, (void*)arg, 8) == 0) {
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
		if (x_copy_from_user((void*)buf, (void*)arg, 8) == 0) {
			struct pid * proc_pid_struct = (struct pid *)*(size_t*)buf;
			size_t rss;
			printk_debug(KERN_INFO "IOCTL_GET_PROCESS_RSS\n");
			printk_debug(KERN_INFO "proc_pid_struct*:0x%p,size:%ld\n", (void*)proc_pid_struct, sizeof(proc_pid_struct));

			memset(&buf, 0, 8);
			rss = read_proc_rss_size(proc_pid_struct);
			memcpy((void*)buf, &rss, 8);

			if (!!x_copy_to_user((void*)arg, &buf, 8))
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
		if (x_copy_from_user((void*)buf, (void*)arg, 8) == 0) {
			struct pid * proc_pid_struct = (struct pid *)*(size_t*)buf;
			size_t arg_start = 0, arg_end = 0;
			int res;

			printk_debug(KERN_INFO "IOCTL_GET_PROCESS_CMDLINE_ADDR\n");

			printk_debug(KERN_INFO "proc_pid_struct *:0x%p,size:%ld\n", (void*)proc_pid_struct, sizeof(proc_pid_struct));

			memset(&buf, 0, 16);

			res = get_proc_cmdline_addr(proc_pid_struct, &arg_start, &arg_end);

			memcpy((void*)buf, &arg_start, 8);
			memcpy((void*)((size_t)buf + (size_t)8), &arg_end, 8);
			if (!!x_copy_to_user((void*)arg, &buf, 16))
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



//MY_STATIC long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
//MY_STATIC long (*compat_ioctl) (struct file *, unsigned int cmd, unsigned long arg);
MY_STATIC long rwProcMem_ioctl(
	struct file *filp,
	unsigned int cmd,
	unsigned long arg) {
	return DispatchCommand(cmd, arg);
}
MY_STATIC loff_t rwProcMem_llseek(struct file* filp, loff_t offset, int orig) {
	unsigned int cmd = 0;
	printk_debug("rwProcMem_llseek offset:%zd\n", (ssize_t)offset);

	if (!!x_copy_from_user((void*)&cmd, (void*)offset, sizeof(unsigned int))) {
		return -EINVAL;
	}
	printk_debug("rwProcMem_llseek cmd:%u\n", cmd);
	return DispatchCommand(cmd, offset + sizeof(unsigned int));
}


#ifdef CONFIG_MODULE_GUIDE_ENTRY
static
#endif
int __init rwProcMem_dev_init(void) {
	int result;
	printk(KERN_EMERG "Start init.\n");
	
	g_rwProcMem_devp = kmalloc(sizeof(struct rwProcMemDev), GFP_KERNEL);
	if (!g_rwProcMem_devp) {
		result = -ENOMEM;
		goto _fail;
	}
	memset(g_rwProcMem_devp, 0, sizeof(struct rwProcMemDev));
	g_rwProcMem_devp->max_dev_open_count = 1;
	g_rwProcMem_devp->is_already_hide_dev_file = false;
	g_rwProcMem_devp->is_already_hide_module_list = false;

#ifdef CONFIG_USE_PROC_FILE_NODE
	g_rwProcMem_devp->proc_entry = proc_create(DEV_FILENAME, S_IRUGO | S_IWUGO, NULL, &rwProcMem_fops);
	if (!g_rwProcMem_devp->proc_entry) {
		printk(KERN_NOTICE "Error in proc_create()\n");
		result = -EFAULT;
		goto _fail;
	}
#endif
#ifdef CONFIG_USE_DEV_FILE_NODE
	result = alloc_chrdev_region(&g_rwProcMem_devno, 0, 1, DEV_FILENAME);
	g_rwProcMem_major = MAJOR(g_rwProcMem_devno);

	if (result < 0) {
		printk(KERN_EMERG "rwProcMem alloc_chrdev_region failed %d\n", result);
		return result;
	}

	g_rwProcMem_devp->pcdev = kmalloc(sizeof(struct cdev) * 3, GFP_KERNEL);
	cdev_init(g_rwProcMem_devp->pcdev, (struct file_operations*)&rwProcMem_fops);
	g_rwProcMem_devp->pcdev->owner = THIS_MODULE;
	g_rwProcMem_devp->pcdev->ops = (struct file_operations*)&rwProcMem_fops;
	if (cdev_add(g_rwProcMem_devp->pcdev, g_rwProcMem_devno, 1)) {
		printk(KERN_NOTICE "Error in cdev_add()\n");
		result = -EFAULT;
		goto _fail;
	}
	g_Class_devp = class_create(THIS_MODULE, DEV_FILENAME);
	device_create(g_Class_devp, NULL, g_rwProcMem_devno, NULL, "%s", DEV_FILENAME);
#endif

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

#ifdef CONFIG_USE_DEV_FILE_NODE
	unregister_chrdev_region(g_rwProcMem_devno, 1);
#endif
	return result;
}

#ifdef CONFIG_MODULE_GUIDE_ENTRY
static
#endif
void __exit rwProcMem_dev_exit(void) {

	printk(KERN_EMERG "Start exit.\n");

#ifdef CONFIG_USE_PROC_FILE_NODE
	proc_remove(g_rwProcMem_devp->proc_entry);
#endif
#ifdef CONFIG_USE_DEV_FILE_NODE
	device_destroy(g_Class_devp, g_rwProcMem_devno); //删除设备文件（位置在/dev/xxxxx）
	class_destroy(g_Class_devp); //删除设备类

	cdev_del(g_rwProcMem_devp->pcdev); //注销cdev
	unregister_chrdev_region(g_rwProcMem_devno, 1); //释放设备号
	kfree(g_rwProcMem_devp->pcdev); // 释放设备结构体内存
#endif
	kfree(g_rwProcMem_devp); // 释放设备结构体内存
	printk(KERN_EMERG "Goodbye, %s\n", DEV_FILENAME);
}

#ifndef CONFIG_MODULE_GUIDE_ENTRY
//Hook:__cfi_check_fn
unsigned char* __check_(unsigned char* result, void *ptr, void *diag)
{
	printk_debug(KERN_EMERG "my__cfi_check_fn!!!\n");
	return result;
}

//Hook:__cfi_check_fail
unsigned char * __check_fail_(unsigned char *result)
{
	printk_debug(KERN_EMERG "my__cfi_check_fail!!!\n");
	return result;
}
#endif

unsigned long __stack_chk_guard;

#ifdef CONFIG_MODULE_GUIDE_ENTRY
module_init(rwProcMem_dev_init);
module_exit(rwProcMem_dev_exit);
#endif

MODULE_AUTHOR("Linux");
MODULE_DESCRIPTION("Linux default module");
MODULE_LICENSE("GPL");

