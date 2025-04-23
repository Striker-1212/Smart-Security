#include <linux/module.h>
#include <linux/poll.h>

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
#include <linux/fcntl.h>
#include <linux/timer.h>

struct gpio_light{
	int gpio;
	struct gpio_desc *gpiod;
	int flag;
	int irq;
	struct timer_list key_timer;
} ;

static struct gpio_light *gpio_lights_100ask;

/* 主设备号                                                                 */
static int major = 0;
static struct class *gpio_light_class;

/* 环形缓冲区 */
#define BUF_LEN 128
static int g_keys[BUF_LEN];
static int r, w;

struct fasync_struct *button_fasync;

#define NEXT_POS(x) ((x+1) % BUF_LEN)

static int is_key_buf_empty(void)
{
	return (r == w);
}

static int is_key_buf_full(void)
{
	return (r == NEXT_POS(w));
}

static void put_key(int key)
{
	if (!is_key_buf_full())
	{
		g_keys[w] = key;
		w = NEXT_POS(w);
	}
}

static int get_key(void)
{
	int key = 0;
	if (!is_key_buf_empty())
	{
		key = g_keys[r];
		r = NEXT_POS(r);
	}
	return key;
}


static DECLARE_WAIT_QUEUE_HEAD(gpio_light_wait);

/* 定时器处理函数，在这个函数中处理数据 */
static void key_timer_expire(unsigned long data)//直接传递地址的数值
{
	/* data ==> gpio */
	struct gpio_light *gpio_light = data;
	int val;
	int key;
	
	val = gpiod_get_value(gpio_light->gpiod);//获得传感器逻辑值
	printk("key_timer_expire key %d %d\n", gpio_light->gpio, val);
	
	key = (gpio_light->gpio << 8) | val;
	put_key(key);//放入环形缓冲区
	
	wake_up_interruptible(&gpio_light_wait);//唤醒等待队列中的read
	//kill_fasync(&button_fasync, SIGIO, POLL_IN);//发送异步信号，read休眠唤醒不用
}


/* 实现对应的open/read/write等函数，填入file_operations结构体                   */
/* read休眠唤醒 */
static ssize_t gpio_light_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	//printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	int err;
	int key;

	if (is_key_buf_empty() && (file->f_flags & O_NONBLOCK))
		return -EAGAIN;
	
	wait_event_interruptible(gpio_light_wait, !is_key_buf_empty());
	key = get_key();
	err = copy_to_user(buf, &key, 4);
	
	return 4;
}

/* poll接口 */
static unsigned int gpio_light_drv_poll(struct file *fp, poll_table * wait)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	poll_wait(fp, &gpio_light_wait, wait);
	return is_key_buf_empty() ? 0 : POLLIN | POLLRDNORM;
}

/* 异步 */
static int gpio_light_drv_fasync(int fd, struct file *file, int on)
{
	if (fasync_helper(fd, file, on, &button_fasync) >= 0)
		return 0;
	else
		return -EIO;
}


/* 定义自己的file_operations结构体                                              */
static struct file_operations gpio_light_drv = {
	.owner	 = THIS_MODULE,
	.read    = gpio_light_drv_read,
	.poll    = gpio_light_drv_poll,
	.fasync  = gpio_light_drv_fasync,
};


/* 中断处理函数中修改定时时间 */
static irqreturn_t gpio_light_isr(int irq, void *dev_id)
{
	struct gpio_light *gpio_light = dev_id;
	printk("gpio_light_isr key %d irq happened\n", gpio_light->gpio);
	mod_timer(&gpio_light->key_timer, jiffies + HZ/5);//100/5*10 = 200ms内若再次有中断触发还会，超时时间重置
	return IRQ_HANDLED;
}

/* 1. 从platform_device获得GPIO
 * 2. gpio=>irq
 * 3. request_irq
 */
static int gpio_light_probe(struct platform_device *pdev)
{
	int err;
	struct device_node *node = pdev->dev.of_node;
	int count;
	int i;
	enum of_gpio_flags flag;
		
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	count = of_gpio_count(node);
	if (!count)
	{
		printk("%s %s line %d, there isn't any gpio available\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

	gpio_lights_100ask = kzalloc(sizeof(struct gpio_light) * count, GFP_KERNEL);
	for (i = 0; i < count; i++)
	{		
		gpio_lights_100ask[i].gpio = of_get_gpio_flags(node, i, &flag);
		if (gpio_lights_100ask[i].gpio < 0)
		{
			printk("%s %s line %d, of_get_gpio_flags fail\n", __FILE__, __FUNCTION__, __LINE__);
			return -1;
		}
		//gpio_lights_100ask[i].gpiod = gpio_to_desc(gpio_lights_100ask[i].gpio);
		
		gpio_lights_100ask[i].gpiod = devm_gpiod_get_index(&pdev->dev, NULL, i, GPIOD_IN);
		gpio_lights_100ask[i].flag = flag & OF_GPIO_ACTIVE_LOW;
		gpio_lights_100ask[i].irq  = gpio_to_irq(gpio_lights_100ask[i].gpio);

		//设置定时器，初始化timer_list结构体，设置其中的函数、参数
		setup_timer(&gpio_lights_100ask[i].key_timer, key_timer_expire, &gpio_lights_100ask[i]);
		gpio_lights_100ask[i].key_timer.expires = ~0;//先给超时时间设置为最大值
		//向内核添加定时器。 timer->expires 表示超时时间。
		add_timer(&gpio_lights_100ask[i].key_timer);//超时时间到达就会调用函数key_timer_expire
	}

	for (i = 0; i < count; i++)
	{
		err = request_irq(gpio_lights_100ask[i].irq, gpio_light_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "100ask_gpio_light", &gpio_lights_100ask[i]);
	}

	/* 注册file_operations 	*/
	major = register_chrdev(0, "100ask_gpio_light", &gpio_light_drv);  /* /dev/gpio_light */

	gpio_light_class = class_create(THIS_MODULE, "100ask_gpio_light_class");
	if (IS_ERR(gpio_light_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "100ask_gpio_light");
		return PTR_ERR(gpio_light_class);
	}

	device_create(gpio_light_class, NULL, MKDEV(major, 0), NULL, "100ask_gpio_light"); /* /dev/100ask_gpio_light */
        
    return 0;
    
}

static int gpio_light_remove(struct platform_device *pdev)
{
	//int err;
	struct device_node *node = pdev->dev.of_node;
	int count;
	int i;

	device_destroy(gpio_light_class, MKDEV(major, 0));
	class_destroy(gpio_light_class);
	unregister_chrdev(major, "100ask_gpio_light");

	count = of_gpio_count(node);
	for (i = 0; i < count; i++)
	{
		free_irq(gpio_lights_100ask[i].irq, &gpio_lights_100ask[i]);
		del_timer(&gpio_lights_100ask[i].key_timer);
	}
	kfree(gpio_lights_100ask);
    return 0;
}


static const struct of_device_id ask100_lights[] = {
    { .compatible = "100ask,gpio_light" },
    { },
};

/* 1. 定义platform_driver */
static struct platform_driver gpio_lights_driver = {
    .probe      = gpio_light_probe,
    .remove     = gpio_light_remove,
    .driver     = {
        .name   = "100ask_gpio_light",
        .of_match_table = ask100_lights,
    },
};

/* 2. 在入口函数注册platform_driver */
static int __init gpio_light_init(void)
{
    int err;
    
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	
    err = platform_driver_register(&gpio_lights_driver); 
	
	return err;
}

/* 3. 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数
 *     卸载platform_driver
 */
static void __exit gpio_light_exit(void)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    platform_driver_unregister(&gpio_lights_driver);
}


/* 7. 其他完善：提供设备信息，自动创建设备节点                                     */

module_init(gpio_light_init);
module_exit(gpio_light_exit);

MODULE_LICENSE("GPL");


