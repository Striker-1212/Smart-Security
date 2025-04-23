#include <linux/module.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>

#include "beep_opr.h"


/* 1. 确定主设备号                                                                 */
static int major = 0;
static struct class *beep_class;
struct beep_operations *p_beep_opr;


#define MIN(a, b) (a < b ? a : b)


void beep_class_create_device(int minor)
{
	device_create(beep_class, NULL, MKDEV(major, minor), NULL, "100ask_beep%d", minor); /* /dev/100ask_beep0,1,... */
}
void beep_class_destroy_device(int minor)
{
	device_destroy(beep_class, MKDEV(major, minor));
}
void register_beep_operations(struct beep_operations *opr)
{
	p_beep_opr = opr;
}

EXPORT_SYMBOL(beep_class_create_device);
EXPORT_SYMBOL(beep_class_destroy_device);
EXPORT_SYMBOL(register_beep_operations);



/* 3. 实现对应的open/read/write等函数，填入file_operations结构体                   */
static ssize_t beep_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

/* write(fd, &val, 1); */
static ssize_t beep_drv_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	int err;
	char status;
	struct inode *inode = file_inode(file);
	int minor = iminor(inode);
	
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	err = copy_from_user(&status, buf, 1);

	/* 根据次设备号和status控制LED */
	p_beep_opr->ctl(minor, status);
	
	return 1;
}

static int beep_drv_open (struct inode *node, struct file *file)
{
	int minor = iminor(node);
	
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	/* 根据次设备号初始化LED */
	p_beep_opr->init(minor);
	
	return 0;
}

static int beep_drv_close (struct inode *node, struct file *file)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

/* 2. 定义自己的file_operations结构体                                              */
static struct file_operations beep_drv = {
	.owner	 = THIS_MODULE,
	.open    = beep_drv_open,
	.read    = beep_drv_read,
	.write   = beep_drv_write,
	.release = beep_drv_close,
};

/* 4. 把file_operations结构体告诉内核：注册驱动程序                                */
/* 5. 谁来注册驱动程序啊？得有一个入口函数：安装驱动程序时，就会去调用这个入口函数 */
static int __init led_init(void)
{
	int err;
	
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	major = register_chrdev(0, "100ask_beep", &beep_drv);  /* /dev/beep */


	beep_class = class_create(THIS_MODULE, "beep_drv_class");
	err = PTR_ERR(beep_class);
	if (IS_ERR(beep_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "100ask_beep");
		return -1;
	}
	
	return 0;
}

/* 6. 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数           */
static void __exit led_exit(void)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	class_destroy(beep_class);
	unregister_chrdev(major, "100ask_beep");
}


/* 7. 其他完善：提供设备信息，自动创建设备节点                                     */

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");


