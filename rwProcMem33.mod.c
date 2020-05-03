#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x4f12492, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x6a2517da, __VMLINUX_SYMBOL_STR(d_path) },
	{ 0xca68d156, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0xa3194c47, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x3cd3c2cd, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0x11c695f5, __VMLINUX_SYMBOL_STR(put_pid) },
	{ 0xfbc74f64, __VMLINUX_SYMBOL_STR(__copy_from_user) },
	{ 0xb27c42d2, __VMLINUX_SYMBOL_STR(up_read) },
	{ 0x67c2fa54, __VMLINUX_SYMBOL_STR(__copy_to_user) },
	{ 0x6d467815, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x2febf438, __VMLINUX_SYMBOL_STR(filp_close) },
	{ 0xdcea0160, __VMLINUX_SYMBOL_STR(mmput) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0xb2628f58, __VMLINUX_SYMBOL_STR(down_read) },
	{ 0xdcb764ad, __VMLINUX_SYMBOL_STR(memset) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x4ba247d1, __VMLINUX_SYMBOL_STR(get_task_mm) },
	{ 0x2455c156, __VMLINUX_SYMBOL_STR(__clear_user) },
	{ 0xf8e398fc, __VMLINUX_SYMBOL_STR(memstart_addr) },
	{ 0x5394e84d, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0x2469810f, __VMLINUX_SYMBOL_STR(__rcu_read_unlock) },
	{ 0x61651be, __VMLINUX_SYMBOL_STR(strcat) },
	{ 0xc7ed30d, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0x11442a11, __VMLINUX_SYMBOL_STR(find_vma) },
	{ 0xf0fdf6cb, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x8a7d1c31, __VMLINUX_SYMBOL_STR(high_memory) },
	{ 0x9bc2c92b, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0xc6d34c3b, __VMLINUX_SYMBOL_STR(find_get_pid) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x2e1da9fb, __VMLINUX_SYMBOL_STR(probe_kernel_read) },
	{ 0x4829a47e, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0x862d9d9f, __VMLINUX_SYMBOL_STR(__put_task_struct) },
	{ 0x5eed0b87, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0xe28db399, __VMLINUX_SYMBOL_STR(get_pid_task) },
	{ 0x8f678b07, __VMLINUX_SYMBOL_STR(__stack_chk_guard) },
	{ 0x28318305, __VMLINUX_SYMBOL_STR(snprintf) },
	{ 0x8d522714, __VMLINUX_SYMBOL_STR(__rcu_read_lock) },
	{ 0x95603012, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0x12d51fd9, __VMLINUX_SYMBOL_STR(filp_open) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

