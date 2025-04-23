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
#include <linux/platform_device.h>
#include <linux/of.h>
#include <asm/io.h>

#include "beep_opr.h"
#include "beepdrv.h"
#include "beep_resource.h"

static int g_beeppins[100];
static int g_beepcnt = 0;

/* registers */
//CCM_CCGR3 CG6(13–12bit) gpio4 clock (gpio4_clk_enable) 地址：0x020C4074,使能gpio4时钟
static volatile unsigned int *CCM_CCGR3;

// SW_MUX_CTL_PAD_CSI_DATA01 地址：0x020E0000 + 0x01E8(GPIO4_22) 0x020E01E8，使能gpio4
static volatile unsigned int *SW_MUX_CTL_PAD_CSI_DATA01;

// GPIO5_GDIR 地址：0x020AC004,参考手册28.5表格中
static volatile unsigned int *GPIO4_GDIR;

//GPIO5_DR 地址：0x020AC000，参考手册28.5表格中
static volatile unsigned int *GPIO4_DR;

/* 硬件操作 */
static int board_beep_init (int which) /* 初始化LED, which-哪个LED */       
{   
    //printk("%s %s line %d, led %d\n", __FILE__, __FUNCTION__, __LINE__, which);
    unsigned int val;
    printk("init gpio: group %d, pin %d\n", GROUP(g_beeppins[which]), PIN(g_beeppins[which]));
    switch(GROUP(g_beeppins[which]))
    {
        case 4:
        {
           //if (!CCM_CCGR3)
	        {
	            CCM_CCGR3                               = ioremap(0x020C4074, 4);
	            SW_MUX_CTL_PAD_CSI_DATA01 				= ioremap(0x020E01E8, 4);
	            GPIO4_GDIR                              = ioremap(0x020A8000 + 0x4, 4);
	            GPIO4_DR                                = ioremap(0x020A8000 + 0, 4);
	        }
	        
	        /* GPIO4_IO22 */
	        /* a. 使能GPIO4 CLK
	         * set CCM to enable GPIO4
	         * CCM_CCGR3[CG6] 0x020C4074
	         * bit[13:12] = 0b11
	         */
	        *CCM_CCGR3 |= (3<<12);
	        
	        /* b. 设置GPIO4_IO022用于GPIO
	         * set SW_MUX_CTL_PAD_CSI_DATA01
	         *      to configure GPIO4_IO22 as GPIO
	         * SW_MUX_CTL_PAD_CSI_DATA01  0x020E01E8
	         * bit[3:0] = 0b0101 alt5
	         */
	        val = *SW_MUX_CTL_PAD_CSI_DATA01;
	        val &= ~(0xf);//低四位清零
	        val |= 5;//设置GPIO4_IO022用于GPIO
	        *SW_MUX_CTL_PAD_CSI_DATA01 = val;
	        
	        
	        /* b. 设置GPIO5_IO03作为output引脚
	         * set GPIO5_GDIR to configure GPIO5_IO03 as output
	         * GPIO4_GDIR  0x020A8000 + 0x4
	         * bit[3] = 0b1
	         */
	        *GPIO4_GDIR |= (1<<22);//设置GPIO4_IO22引脚为输出
            break;
        }
        case 1:
        {
            printk("init pin of group 1 ...\n");
            break;
        }
        case 2:
        {
            printk("init pin of group 2 ...\n");
            break;
        }
        case 3:
        {
            printk("init pin of group 3 ...\n");
            break;
        }
    }
    
    return 0;
}

static int board_beep_ctl (int which, char status) /* 控制LED, which-哪个LED, status:1-亮,0-灭 */
{
    //printk("%s %s line %d, led %d, %s\n", __FILE__, __FUNCTION__, __LINE__, which, status ? "on" : "off");
    printk("set led %s: group %d, pin %d\n", status ? "on" : "off", GROUP(g_beeppins[which]), PIN(g_beeppins[which]));

    switch(GROUP(g_beeppins[which]))
    {
        case 4:
        {
            if (status) /* on: output 0*/
	        {
	            /* d. 设置GPIO4_DR输出低电平(beep低电平触发)
	             * set GPIO4_DR to configure GPIO4_IO22 output 0
	             * GPIO4_DR 0x020A8000 + 0
	             * bit[22] = 0b0
	             */
	            *GPIO4_DR &= ~(1<<22);
	        }
	        else  /* off: output 1*/
	        {
	            /* e. 设置GPIO5_IO3输出高电平
	             * set GPIO4_DR to configure GPIO4_IO22 output 1
	             * GPIO4_DR 0x020A8000 + 0
	             * bit[22] = 0b1
	             */ 
	            *GPIO4_DR |= (1<<22);
	        }
            break;
        }
        case 1:
        {
            printk("set pin of group 1 ...\n");
            break;
        }
        case 2:
        {
            printk("set pin of group 2 ...\n");
            break;
        }
        case 3:
        {
            printk("set pin of group 3 ...\n");
            break;
        }
    }

    return 0;
}

static struct beep_operations board_beep_opr = {
    .init = board_beep_init,
    .ctl  = board_beep_ctl,
};

struct beep_operations *get_board_beep_opr(void)
{
    return &board_beep_opr;
}

static int chip_beep_gpio_probe(struct platform_device *pdev)
{
    struct device_node *np;//设备节点device_node
    int err = 0;
    int beep_pin;

    np = pdev->dev.of_node;//从platform_device中得到device_node
    if (!np)
        return -1;

    err = of_property_read_u32(np, "pin", &beep_pin);//读取设备树pin属性的值，保存在led_pin
    
    g_beeppins[g_beepcnt] = beep_pin;
    beep_class_create_device(g_beepcnt);
    g_beepcnt++;
        
    return 0;
    
}

static int chip_beep_gpio_remove(struct platform_device *pdev)
{
    int i = 0;
    int err;
    struct device_node *np;
    int led_pin;

    np = pdev->dev.of_node;
    if (!np)
        return -1;

    err = of_property_read_u32(np, "pin", &led_pin);

    for (i = 0; i < g_beepcnt; i++)
    {
        if (g_beeppins[i] == led_pin)
        {
            beep_class_destroy_device(i);
            g_beeppins[i] = -1;
            break;
        };
    }

    for (i = 0; i < g_beepcnt; i++)
    {
        if (g_beeppins[i] != -1)
            break;
    }

    if (i == g_beepcnt)//若所有的设备都卸载了，g_beepcnt = 0
        g_beepcnt = 0;
    
    return 0;
}

/* 构造drv的of_match_table */
static const struct of_device_id ask100_beep[] = {
    { .compatible = "100ask,beepdrv" },
    { },
};

static struct platform_driver chip_beep_gpio_driver = {
    .probe      = chip_beep_gpio_probe,
    .remove     = chip_beep_gpio_remove,
    .driver     = {
        .name   = "100ask_beep",//驱动名
        .of_match_table = ask100_beep,//of_match_table变量名
    },
};

static int __init chip_beep_gpio_drv_init(void)
{
    int err;
    
    err = platform_driver_register(&chip_beep_gpio_driver); 
    register_beep_operations(&board_beep_opr);
    
    return 0;
}

static void __exit chip_beep_gpio_drv_exit(void)
{
    platform_driver_unregister(&chip_beep_gpio_driver);
}

module_init(chip_beep_gpio_drv_init);
module_exit(chip_beep_gpio_drv_exit);

MODULE_LICENSE("GPL");

