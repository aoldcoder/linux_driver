#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>

#define CHARDEVBASE_MAJOR   200
#define CHARDEVBASE_NAME    "chardevbase"       /*设备名*/

static char readbuf[100];                       /*读缓冲区*/
static char writebuf[100];                      /*写缓冲区*/
static char kerneldata[]={"kernel data!"};

/**
 *      打开设备 
 *      传递给驱动的inode
 *      设备文件 file结构体有个叫private_data的成员变量
 *      一般在open的时候将private_data指向设备结构体
 *      0 成功; 其它返回
 */

static int chardevbase_open(struct inode *inode,struct file *flip)
{
    /* data */
    return 0;
};

/**
 *  从设备读取数据
 *  要从打开的设备文件
 *  返回给用户空间的数据缓冲区
 *  要读取数据长度
 *  相对于文件首地址的偏移
 *  读取的字节数，如果为负值，表示读取失败 
 * 
 **/  
static ssize_t chardevbase_read(struct file *flip,char __user *buf,size_t cnt,loff_t *offt)
{
    int retvalue = 0;
    /*向用户用户空间发送数据*/
    memcpy(readbuf,kerneldata,sizeof(kerneldata));
    retvalue = copy_to_user(buf,readbuf,cnt);
    if(retvalue == 0){
        printk("kernel senddata ok! \r\n");
    }else{
        printk("kernel senddata failed! \r\n");
    }
    // printk("chardevbase read! %d\r\n",retvalue);
    return 0;
}

/**
 *  向设备写数据
 *  设备文件，表示打开的文件描述符
 *  要写给设备写入的数据
 *  要写入的数据长度
 *  相对于文件首地址的偏移
 *  写入的字节数，如果为负值，表示写入失败
 * 
 */ 
static ssize_t chardevbase_write(struct file *flip,const char __user *buf,size_t cnt,loff_t *offt)
{
     printk("chardevbase write!\r\n");
    int retvalue = 0;
    /*接收用户空间传递给内核的数据并且打印出来*/
    retvalue = copy_from_user(writebuf,buf,cnt);
    if(retvalue == 0){
        printk("kernel recevdata:%s \r\n",writebuf);
    }else{
        printk("kernel recevdata failed! \r\n");
    }
    // printk("chardevbase write! %s\r\n",writebuf);
    return 0;
}

/**
 *  关闭/释放设备
 *  要关闭的设备文件
 *  0成功，其它失败
 */ 
static int chardevbase_release(struct inode *inode,struct file *filp)
{
    /* data */
    return 0;
}

/**
 *  设备操作函数结构体
 */ 
static struct file_operations chardevbase_fops = {
    /* data */
    .owner = THIS_MODULE,
    .open = chardevbase_open,
    .read = chardevbase_read,
    .write = chardevbase_write,
    .release = chardevbase_release,
};


/**
 *  驱动入口函数
 *  无
 *  0成功，其它失败
 */ 

static int __init chardevbase_init(void)
{
    int retvalue = 0;
    /*注册字符设备驱动*/
    retvalue = register_chrdev(CHARDEVBASE_MAJOR,CHARDEVBASE_NAME,&chardevbase_fops);
    if(retvalue <0){
        printk("chardevbase drive register failed \r\n");
    }
    printk("chardevbase_init()\r\n");


    return 0;
}

/**
 *  驱动出口函数 
 * 
 */ 
static void __exit chardevbase_exit(void)
{
    /*注销字符设备驱动*/
    unregister_chrdev(CHARDEVBASE_MAJOR,CHARDEVBASE_NAME);
    printk("chardevbase_exit()\r\n");
}

/**
 *  将上面两个函数指定为驱动的入口和出口函数 
 */ 
module_init(chardevbase_init);
module_exit(chardevbase_exit);

/**
 * LICENSE和作者信息
 */ 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CXQ");
