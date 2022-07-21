#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#define u8              unsigned char
#define u32             unsigned int

#define NEWCHRLED_CNT   1               /*设备号个数*/
#define NEWCHRLED_NAME  "newchrled"     /*名字*/
#define LEDOFF          0               /*关灯*/
#define LEDON           1               /*开灯*/

/*寄存器物理地址*/
#define CCM_CCGR1_BASE              (0X020C406C)
#define SW_MUX_GPIO1_IO03_BASE      (0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE      (0X020E02F4)
#define GPIO1_DR_BASE               (0X0209C000)
#define GPIO1_GDIR_BASE             (0X0209C004)

/*映射后的寄存器虚拟地址指针*/
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

/*newchrled设备结构体*/
struct newchrled_dev{
    dev_t devid;            /*设备号*/
    struct cdev cdev;       /*cdev*/
    struct class *class;    /*类*/
    struct device *device;  /*设备*/
    int major;              /*主设备号*/
    int minjor;             /*次设备号*/
};

struct newchrled_dev newchrled; /*led设备*/

/**
 * LED 打开/关闭
 * LEDON(0) 打开LED,LEDOFF(1)关闭LED
 * 
 */
void led_switch(u8 sta)
{
    u32 val = 0;
    if(sta == LEDON){
        val = readl(GPIO1_DR);
        val &= ~(1<<3);
        writel(val,GPIO1_DR);
    }else if(sta == LEDOFF){
        val = read1(GPIO1_DR);
        val |= (1<<3);
        writel(val,GPIO1_DR);
    }
}

/**
 * 
 * 
 */ 

