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
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>


struct gpio_flame{
	int gpio;
	struct gpio_desc *gpiod;//引脚描述信息
	int flag;
	int irq;//引脚中断号
} ;

static struct gpio_flame *gpio_flames_100ask;//只用一个gpio引脚不需要分配多个内存空间了


/* 主设备号                                                                 */
static int major = 0;
static struct class *gpio_flame_class;

static int g_key = 0;

static DECLARE_WAIT_QUEUE_HEAD(gpio_flame_wait); //声明和初始化一个等待队列头。

/* 实现对应的open/read/write等函数，填入file_operations结构体                   */
static ssize_t gpio_flame_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	//printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	int err;

	/* 中断处理函数会唤醒该内核read进程 */
	wait_event_interruptible(gpio_flame_wait, g_key);//如果 g_key 为假（即 0），进程就会进入挂起状态，但硬件中断可打断
	err = copy_to_user(buf, &g_key, 4);
	g_key = 0;
	
	return 4;
}


/* 定义自己的file_operations结构体                                              */
static struct file_operations gpio_flame_drv = {
	.owner	 = THIS_MODULE,
	.read    = gpio_flame_drv_read,
};


static irqreturn_t gpio_flame_isr(int irq, void *dev_id)
{
	struct gpio_flame *gpio_flame = dev_id;//从request_irq传递过来的参数，接收它
	int val;
	val = gpiod_get_value(gpio_flame->gpiod);//读取 GPIO 引脚的电平状态,gpiod_get_value返回的是物理？逻辑值。
	

	printk("key %d %d\n", gpio_flame->gpio, val);//打印引脚号，引脚电平值
	g_key = (gpio_flame->gpio << 8) | val;//将这两个值组合为一个32位值，只有低16位有效
	wake_up_interruptible(&gpio_flame_wait);//唤醒在等待队列 gpio_flame_wait 中的进程。
	
	return IRQ_HANDLED;
}

/* 1. 从platform_device获得GPIO
 * 2. gpio=>irq
 * 3. request_irq
 */
static int gpio_flame_probe(struct platform_device *pdev)
{
	int err;
	struct device_node *node = pdev->dev.of_node;//没有生成platform_device,从dev.of_node链表中取出该设备节点
	int count;
	int i;
	enum of_gpio_flags flag;
		
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	count = of_gpio_count(node);//设备节点有几个关联的gpio引脚？
	if (!count)
	{
		printk("%s %s line %d, there isn't any gpio available\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

	/* GFP_KERNEL 是一个标志，表示在内核空间中进行内存分配。 */
	gpio_flames_100ask = kzalloc(sizeof(struct gpio_flame) * count, GFP_KERNEL);//分配count个引脚描述结构体内存空间
	for (i = 0; i < count; i++)
	{
		/* 获得引脚和flag */
		gpio_flames_100ask[i].gpio = of_get_gpio_flags(node, i, &flag);//返回非0表示成功，负数出错
		if (gpio_flames_100ask[i].gpio < 0)
		{
			printk("%s %s line %d, of_get_gpio_flags fail\n", __FILE__, __FUNCTION__, __LINE__);
			return -1;
		}
		/* 直接使用 gpiod_get_value() 返回的是原始物理值
		 * 设备树标志没有被正确应用到GPIO描述符中
		 */
		//gpio_flames_100ask[i].gpiod = gpio_to_desc(gpio_flames_100ask[i].gpio);//根据引脚号获得引脚描述信息
		// 正确方式（自动处理GPIO_ACTIVE_LOW等标志）
		gpio_flames_100ask[i].gpiod = devm_gpiod_get_index(&pdev->dev, NULL, i, GPIOD_IN);
		
		gpiod_direction_input(gpio_flames_100ask[i].gpiod);//设置了中断内核可能自动配置引脚方向为输入，所以不配置也能正常读取数据，最好显示配置，避免出问题
		gpio_flames_100ask[i].flag = flag & OF_GPIO_ACTIVE_LOW;//结果为1则为OF_GPIO_ACTIVE_LOW，否则OF_GPIO_SINGLE_ENDED
		gpio_flames_100ask[i].irq  = gpio_to_irq(gpio_flames_100ask[i].gpio);//获得中断号
	}

	for (i = 0; i < count; i++)
	{
		/* 第五个参数用来向中断处理函数传参 ，
   		 * name字段，如"100ask_gpio_flame"，与register_chrdev()名称相同
		 */
		err = request_irq(gpio_flames_100ask[i].irq, gpio_flame_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "100ask_gpio_flame", &gpio_flames_100ask[i]);
	}

	/* 注册file_operations 	*/
	major = register_chrdev(0, "100ask_gpio_flame", &gpio_flame_drv);  /* /dev/100ask_gpio_flame */

	gpio_flame_class = class_create(THIS_MODULE, "100ask_gpio_flame_class");
	if (IS_ERR(gpio_flame_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "100ask_gpio_flame");
		return PTR_ERR(gpio_flame_class);
	}

	device_create(gpio_flame_class, NULL, MKDEV(major, 0), NULL, "100ask_gpio_flame"); /* /dev/100ask_gpio_flame */
        
    return 0;
    
}

static int gpio_flame_remove(struct platform_device *pdev)
{
	//int err;
	struct device_node *node = pdev->dev.of_node;
	int count;
	int i;

	device_destroy(gpio_flame_class, MKDEV(major, 0));
	class_destroy(gpio_flame_class);
	unregister_chrdev(major, "100ask_gpio_flame");

	count = of_gpio_count(node);
	for (i = 0; i < count; i++)
	{
		////解除设备和它的中断处理程序（ISR）之间的绑定。通知内核不再处理中断请求（IRQ），并释放相关资源。
		free_irq(gpio_flames_100ask[i].irq, &gpio_flames_100ask[i]);
	}
	kfree(gpio_flames_100ask);//释放内核引脚结构体内存空间
    return 0;
}


static const struct of_device_id ask100_flames[] = {
    { .compatible = "100ask,gpio_flame" },
    { },
};

/* 1. 定义platform_driver */
static struct platform_driver gpio_flames_driver = {
    .probe      = gpio_flame_probe,
    .remove     = gpio_flame_remove,
    .driver     = {
        .name   = "100ask_gpio_flame",
        .of_match_table = ask100_flames,
    },
};

/* 2. 在入口函数注册platform_driver */
static int __init gpio_flame_init(void)
{
    int err;
    
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	
    err = platform_driver_register(&gpio_flames_driver); //注册驱动opr，包括probe，remove等操作
	
	return err;
}

/* 3. 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数
 *     卸载platform_driver
 */
static void __exit gpio_flame_exit(void)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    platform_driver_unregister(&gpio_flames_driver);
}


/* 7. 其他完善：提供设备信息，自动创建设备节点                                     */

module_init(gpio_flame_init);
module_exit(gpio_flame_exit);

MODULE_LICENSE("GPL");


