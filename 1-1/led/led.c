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
#include <asm/uaccess.h>
#include <asm/io.h>

#define NEWCHRLED_CNT           1   /*设备号个数*/
#define NEWCHRLED_NAME          "newchrled" /*名字*/
#define LEDOFF                  0   /*开灯*/
#define LEDON                   1   /*关灯*/


/* 寄存器物理地址 */
 #define CCM_CCGR1_BASE (0X020C406C) 
 #define SW_MUX_GPIO1_IO03_BASE (0X020E0068)
 #define SW_PAD_GPIO1_IO03_BASE (0X020E02F4)
 #define GPIO1_DR_BASE (0X0209C000)
 #define GPIO1_GDIR_BASE (0X0209C004)


  /* 映射后的寄存器虚拟地址指针 */
 static void __iomem *IMX6U_CCM_CCGR1;
 static void __iomem *SW_MUX_GPIO1_IO03;
 static void __iomem *SW_PAD_GPIO1_IO03;
 static void __iomem *GPIO1_DR;
 static void __iomem *GPIO1_GDIR;

 /*newchrled设备结构体*/
 struct newchrled_dev{
    dev_t devid;        //设备号
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
 };

 struct newchrled_dev newchrled;


void led_switch(u8 sta)
{
    u32 val = 0;
    if(sta == LEDON)
    {
        val = readl(GPIO1_DR);
        val &= !(1<<3);
        writel(val,GPIO1_DR);
    }else if(sta == LEDOFF){
        val = readl(GPIO1_DR);
        val |= (1<<3);
        writel(val,GPIO1_DR);
    }
}

static int led_open(struct inode *inode,struct file *filp)
{
    filp->private_data = &newchrled;    //设置私有数据
    return 0;

}

static ssize_t led_read(struct file *filp,char __user *buf,size_t cnt,loff_t *offt)
{
    return 0;
}

static ssize_t led_write (struct file *filp,const char __user *buf,size_t cnt,loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char ledstat;

    retvalue = copy_from_user(databuf,buf,cnt);
    if(retvalue <= 0)
    {
        printk("kernel write failed \r\n");
        return -EFAULT;
    }
    ledstat = databuf[0];
    if(ledstat == LEDON)
    {
        led_switch(LEDON);
    }else if(ledstat == LEDOFF)
    {
        led_switch(LEDOFF);
    }
    return 0;
}

static int led_release(struct inode *inode,struct file *filp)
{
    return 0;
}

static struct file_operations newchrled_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
};


struct class *class;        /*类*/
struct device *device;      /*设备*/
dev_t devid;                /*设备号*/

/**驱动入口函数*/
static int __init led_init(void)
{
    // /*创建类*/
    // class = class_create(THIS_MODULE,"LED");
    // /*创建设备*/
    // device = device_create(class,NULL,devid,NULL,"led");

    u32 val = 0;
    /*初始化LED*/

    /* 初始化 LED */
 /* 1、寄存器地址映射 */
//  IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
//  SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
//  SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
//  GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);
//  GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 4);


 /* 2、使能 GPIO1 时钟 */
 val = readl(IMX6U_CCM_CCGR1);
 val &= ~(3 << 26); /* 清楚以前的设置 */
 val |= (3 << 26); /* 设置新值 */
 writel(val, IMX6U_CCM_CCGR1);

/* 3、设置 GPIO1_IO03 的复用功能，将其复用为
 * GPIO1_IO03，最后设置 IO 属性。
 */
 writel(5,SW_MUX_GPIO1_IO03);

/*寄存器SW_PAD_GPIO1_IO03*/
writel(0x10b0,SW_PAD_GPIO1_IO03);

/*设置GPIO1_IO03为输出功能*/

val = readl(GPIO1_GDIR);
val &= ~(1 <<3);
val |= (1<<3);
writel(val,GPIO1_GDIR);

/* 5、默认关闭 LED */
 val = readl(GPIO1_DR);
 val |= (1 << 3); 
 writel(val, GPIO1_DR);

 /*注册字符设备驱动*/
 if(newchrled.major){
    newchrled.devid = MKDEV(newchrled.major,0);
    register_chrdev_region(newchrled.devid,NEWCHRLED_CNT,NEWCHRLED_NAME);
 }else{
    alloc_chrdev_region(&newchrled.devid,0,NEWCHRLED_CNT,NEWCHRLED_NAME);
    newchrled.major = MAJOR(newchrled.devid);
    newchrled.minor = MINOR(newchrled.devid);
 }
 printk("newchrled major = %d,minor =%d\r\n",newchrled.major,newchrled.minor);

 /*初始化cdev*/
newchrled.cdev.owner = THIS_MODULE;
 cdev_init(&newchrled.cdev,&newchrled_fops);

 cdev_add(&newchrled.cdev,newchrled.devid,NEWCHRLED_CNT);

 newchrled.class = class_create(THIS_MODULE,NEWCHRLED_NAME);
 if(IS_ERR(newchrled.class)){
    return PTR_ERR(newchrled.class);
 }


    return 0;
}

static void __exit led_exit(void)
{
    /*删除设备*/
    device_destroy(newchrled.class,newchrled.devid);
    /*删除类*/
    class_destroy(newchrled.class);
}
module_init(led_init);
module_exit(led_exit);

 /* 
 * LICENSE 和作者信息
 */
 MODULE_LICENSE("GPL");
 MODULE_AUTHOR("zuozhongkai");
  