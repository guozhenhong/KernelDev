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
	{ 0x35ec255d, "module_layout" },
	{ 0x15692c87, "param_ops_int" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0xebd5feb1, "cdev_add" },
	{ 0xb2c831b6, "cdev_init" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x2f35ab80, "cdev_del" },
	{ 0x2f287f0d, "copy_to_user" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0xc4554217, "up" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x4792c572, "down_interruptible" },
	{ 0x41ad0272, "kmem_cache_alloc_trace" },
	{ 0xcea0a119, "kmalloc_caches" },
	{ 0x37a0cba, "kfree" },
	{ 0x50eedeb8, "printk" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "1AAD4DA82B83547805A09AE");
