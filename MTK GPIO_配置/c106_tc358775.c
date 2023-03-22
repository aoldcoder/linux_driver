#ifndef BUILD_LK
#include <linux/string.h>
#else
#include <string.h>
#endif 

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#elif (defined BUILD_UBOOT)
#include <asm/arch/mt6577_gpio.h>
#else
	#include "upmu_common.h"
    #include <mt-plat/mt_gpio.h>
	//#include <mach/gpio_const.h>
	#include <mt-plat/mtk_thermal_typedefs.h>
#endif
// tong add for  DSI_clk_HS_mode
#include "ddp_hal.h"  
#include "lcm_drv.h"


#include <platform/mt_pwm.h>
#include <platform/boot_mode.h>
#include <platform/ddp_pwm.h>
#include <mt_disp_drv.h>


#define GPIO_LT8911B_PWR	    	(GPIO43 | 0x80000000)
//#define GPIO_LCM_BL_EN 				(GPIO122 | 0x80000000)  
//#define GPIO_LCM_PWR  				(GPIO43 | 0x80000000)
//#define GPIO_LCM_RST				(GPIO19 | 0x80000000)
#define GPIO_LVDS_RST				(GPIO83 | 0x80000000)
#define GPIO_LVDS_STBY				(GPIO31 | 0x80000000)
//#define GPIO_FAN_EN					(GPIO74 | 0x80000000)
#define GPIO_TEC_EN					(GPIO48 | 0x80000000)


#define GPIO_FAN_RL_SW				(GPIO56 | 0x80000000)
#define GPIO_FAN_UD_SW				(GPIO46 | 0x80000000)
#define GPIO_LED_EN_SW				(GPIO30 | 0x80000000)			//待修改成23


#define LCM_EN_0		(GPIO32 | 0x80000000)
#define LCM_EN_1		(GPIO59 | 0x80000000)

#define GPIO_POWER_EN	(GPIO30	| 0x80000000)
#define GPIO_PROJ_EN	(GPIO32	| 0x80000000)
#define GPIO_DCDC_EN	(GPIO86	| 0x80000000)




//#define GPIO_LVDS_LCD_VDD				(GPIO43 | 0x80000000)

extern void DSI_clk_HS_mode(DISP_MODULE_ENUM module, void *cmdq, bool enter);
extern int disp_pwm_set_backlight(disp_pwm_id_t id, int level_1024);

#define REGFLAG_DELAY       	0xFFFC
#define REGFLAG_UDELAY  		0xFFFB
#define REGFLAG_END_OF_TABLE    0xFFFD
	
	struct LCM_setting_table {
		unsigned int cmd;
		unsigned char count;
		unsigned char para_list[64];
	};

#ifdef BUILD_LK
	#define LT8911_LOG printf
	#define LT8911_REG_WRITE(add, data ,len) mt8193_reg_i2c_write(add, data ,len)
	#define LT8911_REG_READ(add) lt8911_reg_i2c_read(add)
#elif (defined BUILD_UBOOT)
#else
	//extern int lt8911_i2c_write(u8 addr, u8 data);
	extern kal_uint8  lt8911_reg_i2c_read(kal_uint8 addr);
	extern int lt8911_reg_i2c_write(kal_uint8 addr, kal_uint8 val);
	#define LT8911_LOG printk
	#define LT8911_REG_WRITE(add, data) lt8911_reg_i2c_write(add, data)
	#define LT8911_REG_READ(add) lt8911_reg_i2c_read(add)
	
	extern int lcm_i2c_suspend(void);
	extern int lcm_i2c_resume(void);
#endif

// #define _8_bit_	// 6 bit color depth
#define _6_bit_
//#define _Test_Pattern_ 
static LCM_UTIL_FUNCS lcm_util = {0};

/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
	lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) \
	lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
	lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
	lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
//#define   __LCD_7_INC__
#define   __LCD_21_INC__

#ifdef __LCD_7_INC__
#define FRAME_WIDTH  (1024)
#define FRAME_HEIGHT (600)
#else
#ifdef __LCD_21_INC__
#define FRAME_WIDTH  (1920)
#define FRAME_HEIGHT (1080)
#else
#define FRAME_WIDTH  (1366)
#define FRAME_HEIGHT (768)
#endif
#endif
//lxp added for test
#define GPIO_SPI_CE (GPIO33|0x80000000)
#define GPIO_SPI_CLK (GPIO37|0x80000000)
#define GPIO_SPI_MISO   (GPIO38|0x80000000)
#define GPIO_SPI_MOSI (GPIO34|0x80000000)
//lxp added for test end
extern void DSI_clk_HS_mode(DISP_MODULE_ENUM module, void *cmdq, bool enter);
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------



#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))
#ifdef BUILD_LK
static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output){
	if(GPIO == 0xFFFFFFFF)  
		{  
		#ifdef BUILD_LK    
		printf("[LK/LCM] invalid gpio\n");	  
		#else	  	
		printk("[LK/LCM] invalid gpio\n");	  
		#endif 
		}       
	mt_set_gpio_mode(GPIO, GPIO_MODE_00);    
	mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);    
	mt_set_gpio_out(GPIO, (output>0)? GPIO_OUT_ONE: GPIO_OUT_ZERO);
}
#else
static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
        gpio_direction_output(GPIO, output);
        gpio_set_value(GPIO, output);
}
#endif
// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{

	memset(params, 0, sizeof(LCM_PARAMS));
		
    params->type   = LCM_TYPE_DSI;

    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    params->dsi.mode    = SYNC_PULSE_VDO_MODE;//SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;
    //params->dsi.LANE_NUM                = LCM_THREE_LANE;


    //params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
	
	
    //params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB666;

    //params->dsi.PS=LCM_PACKED_PS_18BIT_RGB666;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 

//add
    //params->dsi.packet_size = 256;
    //params->lcm_if = LCM_INTERFACE_DSI0;
    //params->lcm_cmd_if = LCM_INTERFACE_DSI0;
    //params->dsi.word_count = FRAME_WIDTH * 3;
	//params->dsi.pll_select = 1;	/* 0: MIPI_PLL; 1: LVDS_PLL */
	params->dsi.cont_clock=1; 
//
#if 0
    params->dsi.vertical_sync_active = 2;//5;
    params->dsi.vertical_backporch = 4;//3;
    params->dsi.vertical_frontporch = 5;//3;
#else
    params->dsi.vertical_sync_active = 10;//5;
    params->dsi.vertical_backporch = 15;//3;
    params->dsi.vertical_frontporch = 16;//3;
#endif
    params->dsi.vertical_active_line = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active = 10;//50;
    params->dsi.horizontal_backporch =  30;//60; 
    params->dsi.horizontal_frontporch = 40;//50;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;

    params->dsi.PLL_CLOCK=332;//276;//408; 
   //   params->dsi.PLL_CLOCK=100;//408; 
	//params->dsi.noncont_clock=0; 

}

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
	unsigned cmd;
	cmd = table[i].cmd;

	switch (cmd) {

		case REGFLAG_DELAY:
		if (table[i].count <= 10)
			MDELAY(table[i].count);
		else
			MDELAY(table[i].count);
		break;

		case REGFLAG_UDELAY:
		UDELAY(table[i].count);
		break;

		case REGFLAG_END_OF_TABLE:
		break;

		default:
		dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		//LCM_LOGI("ST7701----table[%d].count=%d----\n", i,table[i].count);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////
#ifdef BUILD_LK
#define I2C_CH                I2C0
#define LT8911_I2C_ADDR      0x1f

u32 _lt8911_i2c_read (U8 chip, U8 *cmdBuffer, int cmdBufferLen, U8 *dataBuffer, int dataBufferLen)
{
    U32 ret_code = I2C_OK;
	U8 len; 
	struct mt_i2c_t i2c;
	
	i2c.id = I2C_CH;
	i2c.addr = (LT8911_I2C_ADDR>>1);
	i2c.mode = ST_MODE;
	i2c.speed = 100;	
	len  = 1; 

	int write_len = cmdBufferLen;
	int read_len = dataBufferLen;
	
	//*dataBuffer = *cmdBuffer;
	ret_code = i2c_write_read(&i2c, dataBuffer, write_len, read_len);
    return ret_code;
}

u32 _lt8911_i2c_write(U8 chip, U8 *cmdBuffer, int cmdBufferLen, U8 *dataBuffer, int dataBufferLen)
{
    U32 ret_code = I2C_OK;
    U8 write_data[I2C_FIFO_SIZE];
    int transfer_len = cmdBufferLen + dataBufferLen;
    int i=0, cmdIndex=0, dataIndex=0;


	U8 len; 
	struct mt_i2c_t i2c;
	
	i2c.id = I2C_CH;
	i2c.addr = (LT8911_I2C_ADDR>>1);
	i2c.mode = ST_MODE;
	i2c.speed = 100;	
	write_data[0] = *cmdBuffer;
	write_data[1] = *dataBuffer;
	len  = 2; 
	
	ret_code = i2c_write(&i2c, write_data, len);
    return ret_code;
}

u32 lt8911_reg_i2c_read(u16 addr)
{
    u8 cmd[2];
	cmd[0] = (addr & 0xff00)>>8; ;
	cmd[1] = addr & 0xff;
    int cmd_len = 2;

    u8 data[4];
	data[0] = cmd[0];
	data[1] = cmd[1];
	data[2] = 0xFF;
	data[3] = 0xFF;
    int data_len = 4;
	u32 result_tmp;
    result_tmp = _lt8911_i2c_read(LT8911_I2C_ADDR, &cmd, cmd_len, &data, data_len);
	int read_data = data[0] | (data[1]<<8) | (data[1]<<16) | (data[1]<<24);		
    return read_data;
}

int lt8911_reg_i2c_write(u8 addr, u8 value)
{
    u8 cmd = addr;
    int cmd_len = 1;
    u8 data = value;
    int data_len = 1;	
    u32 result_tmp;

    result_tmp = _lt8911_i2c_write(LT8911_I2C_ADDR, &cmd, cmd_len, &data, data_len);
		return result_tmp;
}

#endif

unsigned long mt_i2c_write (unsigned char channel,unsigned char chip, unsigned char *buffer, int len, unsigned char dir)
{
    U32 ret_code = 0;
    static struct mt_i2c_t i2c;
    i2c.id = channel;
    i2c.addr = chip>>1;	
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    ret_code = i2c_write(&i2c, buffer, len);
    //ret_code = mt_i2c_write(I2C_CH, MT8193_I2C_ADDR, buffer, lens, 0); // 0:I2C_PATH_NORMAL
    return ret_code;
}

unsigned long mt_i2c_read(unsigned char channel,unsigned char chip, unsigned char *buffer, int len , unsigned char dir)
{
    U32 ret_code = 0;
    static struct mt_i2c_t i2c;
    i2c.id = channel;
    i2c.addr = chip>>1;	
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    ret_code = i2c_read(&i2c, buffer, len);
    return ret_code;
}

u32 mt8193_reg_i2c_read(u16 slave_addr,u16 addr)
{
    
    u8 buffer[8] = {0};
    u8 lens;
    u32 ret_code = 0;
    u32 data;

    /*if(((addr >> 8) & 0xFF) >= 0x80) // 8 bit : fast mode
    {
        buffer[0] = (addr >> 8) & 0xFF;
        lens = 1;
    }
    else // 16 bit : noraml mode
    {*/
        buffer[0] = ( addr >> 8 ) & 0xFF;
        buffer[1] = addr & 0xFF;     
        lens = 2;
    //}

	printf("LK %s mt_i2c_write  buffer[0] = 0x%x buffer[1] = 0x%x lens = %d\r\n", __func__,buffer[0],buffer[1],lens);

    ret_code = mt_i2c_write(I2C_CH, slave_addr, buffer, lens, 0);    // set register command
    if (ret_code != 0){
		printf("LK %s mt_i2c_write  \r\n", __func__);
        return ret_code;
	}
    lens = 4;
    ret_code = mt_i2c_read(I2C_CH, slave_addr, buffer, lens, 0);
    if (ret_code != 0)
    {
		printf("LK %s mt_i2c_read  \r\n", __func__);
        return ret_code;
    }
    
    data = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | (buffer[0]); //LSB fisrt

	printf("LK %s mt_i2c_read  data = %x \r\n", __func__,data);

    return data;
    
}

u32 mt8193_reg_i2c_write(u16 addr, u32 data , u8 len)
{
    u8 buffer[8];
    u8 lens;
	U32 ret_code = 0;

	buffer[0] = (addr >> 8) & 0xFF;
	buffer[1] = addr & 0xFF;
	buffer[2] = data & 0xFF;     
	buffer[3] = (data >> 8) & 0xFF;
	buffer[4] = (data >> 16) & 0xFF;	
	buffer[5] = (data >> 24) & 0xFF;		
	lens = 6;	

	ret_code = mt_i2c_write(I2C_CH, LT8911_I2C_ADDR, buffer, lens, 0); // 0:I2C_PATH_NORMAL
    if (ret_code != 0)
    {
        printf("[LK/LCM] mt_i2c_write reg[0x%X] fail, Error code [0x%X] \n", addr, ret_code);
        return ret_code;
    }
	
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/*
设置GPIO17复用为PWM功能 
PWM_CLK_OLD_MODE_BLOCK=26MHZ
输出频率为：clk_src/clk_div/DATA_WIDTH
26000/1/100约等于260kHZ
占空比为:THRESH/DATA_WIDTH
50/100=1/2
PWM工作模式有5种：OLD MODE/FIFO MODE/MEMORY MODE/RANDOM MODE/SEQ MODE
*/
void gpio_set_pwm(int pwm_num, u32 data)
{   
		#if 0      
    struct pwm_spec_config pwm_setting;
 
    memset(&pwm_setting, 0, sizeof(struct pwm_spec_config));
    pwm_setting.pwm_no = pwm_num;
    pwm_setting.mode = PWM_MODE_OLD;
    printf("my_set_pwm  pwm_no=%d\n",pwm_num);
   
    pwm_setting.clk_src = PWM_CLK_OLD_MODE_BLOCK; //pwm_setting.pmic_pad = 0;
                                                  //PWM_BLCK和DDR频率有关
    pwm_setting.pwm_mode.PWM_MODE_OLD_REGS.THRESH = duty; 
    pwm_setting.clk_div = CLK_DIV1;//CLK_DIV1 = 1，分频系数
    pwm_setting.pwm_mode.PWM_MODE_OLD_REGS.DATA_WIDTH = 100;
 
    pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.IDLE_VALUE = 0;
    pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.GUARD_VALUE = 0;
    pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.GDURATION = 0;
    pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;
    pwm_set_spec_config(&pwm_setting);
    #endif
    
    #if 1
    struct pwm_spec_config pwm_setting;	
		pwm_setting.pwm_no = pwm_num;
		pwm_setting.mode = PWM_MODE_FIFO; //new mode fifo and periodical mode
		pwm_setting.clk_src = PWM_CLK_NEW_MODE_BLOCK;//52MHz
    pwm_setting.clk_div = CLK_DIV8; // 52MHz/128
    pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.IDLE_VALUE = 0;

		 
    pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.GUARD_VALUE = 0;
    pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 31; // 52MHz/128/64
    pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.HDURATION = 3;
    pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.LDURATION = 3;// 52MHz/128/64/(3+3)≈1KHz
    pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.GDURATION = 0;
    pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;
    pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.SEND_DATA0 = data;//0xFFFFFFFF; // duty is 50%
    pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.SEND_DATA1 = 0x00000000;
	
		pwm_set_spec_config(&pwm_setting);
    #endif
         
}

static struct LCM_setting_table init_setting[] = {
	{0x3C, 5, {0x01,0x08,0x00,0x06,0x00}},
	{0x14, 5, {0x01,0x05,0x00,0x00,0x00}},
	{0x64, 5, {0x01,0x06,0x00,0x00,0x00}},
	{0x68, 5, {0x01,0x06,0x00,0x00,0x00}},
	{0x6C, 5, {0x01,0x06,0x00,0x00,0x00}},
	{0x70, 5, {0x01,0x06,0x00,0x00,0x00}},
	{0x34, 5, {0x01,0x1F,0x00,0x00,0x00}},
	{0x10, 5, {0x02,0x1F,0x00,0x00,0x00}},
	{0x04, 5, {0x01,0x01,0x00,0x00,0x00}},
	{0x04, 5, {0x02,0x01,0x00,0x00,0x00}},

	{0x50, 5, {0x04,0x00,0x01,0xF0,0x03}},
	{0x54, 5, {0x04,0x0A,0x00,0x1E,0x00}},
	{0x58, 5, {0x04,0x80,0x07,0x28,0x00}},
	{0x5C, 5, {0x04,0x0A,0x00,0x0F,0x00}},
	{0x60, 5, {0x04,0x38,0x04,0x0F,0x00}},
	{0x64, 5, {0x04,0x01,0x00,0x00,0x00}},
	{0xA0, 5, {0x04,0x2D,0x80,0x44,0x00}},
	{REGFLAG_DELAY,8,{}},
	{0xA0, 5, {0x04,0x2D,0x80,0x04,0x00}},
	{0x04, 5, {0x05,0x04,0x00,0x00,0x00}},
	
	
	{0x80, 5, {0x04,0x00,0x01,0x02,0x03}},
	{0x84, 5, {0x04,0x04,0x07,0x05,0x08}},
	{0x88, 5, {0x04,0x09,0x0A,0x0E,0x0F}},
	{0x8C, 5, {0x04,0x0B,0x0C,0x0D,0x10}},
	{0x90, 5, {0x04,0x16,0x17,0x11,0x12}},
	{0x94, 5, {0x04,0x13,0x14,0x15,0x1B}},
	{0x98, 5, {0x04,0x18,0x19,0x1A,0x06}},
	
	{0x9C, 5, {0x04,0x33,0x04,0x00,0x00}},
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
	static void init_tc358775_registers(void)
	{

	unsigned int data_array[16];

	data_array[0] = 0x00062902;
	data_array[1] = 0x0008013C;
	data_array[2] = 0x00000006;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x00050114;
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x00060164;
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x00060168;
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x0006016C;
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x00060170;
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x001F0134;
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x001F0210;
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x00010104;
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x00010204;
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x01000450;
	data_array[2] = 0x000003F0;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x000a0454;
	data_array[2] = 0x0000001E;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x07800458;
	data_array[2] = 0x00000028;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x000A045C;
	data_array[2] = 0x0000000F;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x04380460;
	data_array[2] = 0x0000000F;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x00010464;
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x802D04A0;
	data_array[2] = 0x00000004;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x802D04A0;
	data_array[2] = 0x00000004;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x00040504;
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x01000480;
	data_array[2] = 0x00000302;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x07040484;
	data_array[2] = 0x00000805;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x0A090488;
	data_array[2] = 0x00000F0E;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x0C0B048C;
	data_array[2] = 0x0000100D;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x17160490;
	data_array[2] = 0x00001211;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x14130494;
	data_array[2] = 0x00001B15;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x19180498;
	data_array[2] = 0x0000061A;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
	
	data_array[0] = 0x00062902;
	data_array[1] = 0x0433049C;
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(2);
}
static void init_lt8911_registers(void)
{
	printf("LK %s enter  \r\n", __func__);
#if 1
//TC358774/75XBG DSI Basic Parameters.  Following 10 setting should be pefromed in LP mode
	LT8911_REG_WRITE(0x013C,0x00060008,6);  //CSH 0x000A000C
	LT8911_REG_WRITE(0x0114,0x00000005,6);  //CSH0x00000008
	LT8911_REG_WRITE(0x0164,0x00000006,6);
	LT8911_REG_WRITE(0x0168,0x00000006,6);
	LT8911_REG_WRITE(0x016C,0x00000006,6);
	LT8911_REG_WRITE(0x0170,0x00000006,6);  //CSH 0x0000000D
	LT8911_REG_WRITE(0x0134,0x0000001F,6);  
	LT8911_REG_WRITE(0x0210,0x0000001F,6);  //输入通道数
	LT8911_REG_WRITE(0x0104,0x00000001,6);
	LT8911_REG_WRITE(0x0204,0x00000001,6);

//TC358774/75XBG Timing and mode setting
//csh	LT8911_REG_WRITE(0x0450,0x03F00100,6);
	LT8911_REG_WRITE(0x0450,0x03F00100,6);
	
	
	LT8911_REG_WRITE(0x0454,0x001e000a,6);  //CSH0x0032001E
	LT8911_REG_WRITE(0x0458,0x00280780,6);  //CSH 0x00500780
	LT8911_REG_WRITE(0x045C,0x000F000a,6);   //0x00040002
	LT8911_REG_WRITE(0x0460,0x000f0438,6);   //CSH
	LT8911_REG_WRITE(0x0464,0x00000001,6);
	LT8911_REG_WRITE(0x04A0,0x0044802d,6);  //CSH
	MDELAY(8);
	LT8911_REG_WRITE(0x04A0,0x0004802d,6);
	LT8911_REG_WRITE(0x0504,0x00000004,6);

//TC358774/75XBG LVDS Color mapping setting
	LT8911_REG_WRITE(0x0480,0x03020100,6);
	LT8911_REG_WRITE(0x0484,0x08050704,6);
	LT8911_REG_WRITE(0x0488,0x0F0E0A09,6);
	LT8911_REG_WRITE(0x048C,0x100D0C0B,6);
	LT8911_REG_WRITE(0x0490,0x12111716,6);
	LT8911_REG_WRITE(0x0494,0x1B151413,6);
	LT8911_REG_WRITE(0x0498,0x061A1918,6);

//TC358774/75XBG LVDS enable
	LT8911_REG_WRITE(0x049C,0x00000433,6);
#endif	
#if  0
	LT8911_REG_WRITE(0x013C,0x00030005,6);  //CSH 0x000A000C
	LT8911_REG_WRITE(0x0114,0x00000003,6);  //CSH0x00000008
	LT8911_REG_WRITE(0x0164,0x00000004,6);
	LT8911_REG_WRITE(0x0168,0x00000004,6);
	LT8911_REG_WRITE(0x016C,0x00000004,6);
	LT8911_REG_WRITE(0x0170,0x00000004,6);  //CSH 0x0000000D
	LT8911_REG_WRITE(0x0134,0x0000001F,6);  
	LT8911_REG_WRITE(0x0210,0x0000001F,6);  //输入通道数
	LT8911_REG_WRITE(0x0104,0x00000001,6);
	LT8911_REG_WRITE(0x0204,0x00000001,6);

//TC358774/75XBG Timing and mode setting
//csh	LT8911_REG_WRITE(0x0450,0x03F00100,6);
	LT8911_REG_WRITE(0x0450,0x03F00000,6);
	
	
	LT8911_REG_WRITE(0x0454,0x003c0014,6);  //CSH0x0032001E
	LT8911_REG_WRITE(0x0458,0x00500556,6);  //CSH 0x00500780
	LT8911_REG_WRITE(0x045C,0x00080004,6);   //0x00040002
	LT8911_REG_WRITE(0x0460,0x000F0300,6);   //CSH
	LT8911_REG_WRITE(0x0464,0x00000001,6);
	LT8911_REG_WRITE(0x04A0,0x00448006,6);  //CSH
	MDELAY(8);
	LT8911_REG_WRITE(0x04A0,0x00048006,6);
	LT8911_REG_WRITE(0x0504,0x00000004,6);

//TC358774/75XBG LVDS Color mapping setting

/*
	LT8911_REG_WRITE(0x0480,0x03020100,6);
	LT8911_REG_WRITE(0x0484,0x08050704,6);
	LT8911_REG_WRITE(0x0488,0x0F0E0A09,6);
	LT8911_REG_WRITE(0x048C,0x100D0C0B,6);
	LT8911_REG_WRITE(0x0490,0x12111716,6);
	LT8911_REG_WRITE(0x0494,0x1B151413,6);
	LT8911_REG_WRITE(0x0498,0x061A1918,6);
*/

//TC358774/75XBG LVDS enable
	LT8911_REG_WRITE(0x049C,0x00000031,6);
#endif
#if   0
	LT8911_REG_WRITE(0x013C,0x00080008,6);  //CSH 0x000A000C
	LT8911_REG_WRITE(0x0114,0x00000005,6);  //CSH0x00000008
	LT8911_REG_WRITE(0x0164,0x00000008,6);
	LT8911_REG_WRITE(0x0168,0x00000008,6);
	LT8911_REG_WRITE(0x016C,0x00000008,6);
	LT8911_REG_WRITE(0x0170,0x00000008,6);  //CSH 0x0000000D
	LT8911_REG_WRITE(0x0134,0x0000001F,6);  
	LT8911_REG_WRITE(0x0210,0x0000001F,6);  //输入通道数
	LT8911_REG_WRITE(0x0104,0x00000001,6);
	LT8911_REG_WRITE(0x0204,0x00000001,6);

//TC358774/75XBG Timing and mode setting
//csh	LT8911_REG_WRITE(0x0450,0x03F00100,6);
	LT8911_REG_WRITE(0x0450,0x03F00000,6);
	
	
	LT8911_REG_WRITE(0x0454,0x003C0014,6);  //CSH0x0032001E
	LT8911_REG_WRITE(0x0458,0x00500556,6);  //CSH 0x00500780
	LT8911_REG_WRITE(0x045C,0x00080004,6);   //0x00040002
	LT8911_REG_WRITE(0x0460,0x000A0300,6);   //CSH
	LT8911_REG_WRITE(0x0464,0x00000001,6);
	LT8911_REG_WRITE(0x04A0,0x0044802D,6);  //CSH
	MDELAY(10);
	LT8911_REG_WRITE(0x04A0,0x0044802D,6);
	LT8911_REG_WRITE(0x0504,0x00000004,6);

//TC358774/75XBG LVDS Color mapping setting
	LT8911_REG_WRITE(0x0480,0x03020100,6);
	LT8911_REG_WRITE(0x0484,0x08050704,6);
	LT8911_REG_WRITE(0x0488,0x0F0E0A09,6);
	LT8911_REG_WRITE(0x048C,0x100D0C0B,6);
	LT8911_REG_WRITE(0x0490,0x12111716,6);
	LT8911_REG_WRITE(0x0494,0x1B151413,6);
	LT8911_REG_WRITE(0x0498,0x061A1918,6);

//TC358774/75XBG LVDS enable
	LT8911_REG_WRITE(0x049C,0x00000431,6);
#endif

	printf("LK %s exit  \r\n", __func__);
}
void lcm_rotate_180_func(int mode)
{
    unsigned char data;
    switch(mode)
    {
        case 0:  //杩樺師鍒濆?嬬姸鎬?
            //data = 0x02;
            //dsi_set_cmdq_V2(0x36, 1, &data, 1);
			lcm_set_gpio_output(GPIO_FAN_UD_SW, 0); 
            break;
        case 1: //鏃嬭浆180掳
            //data = 0x01;
            //dsi_set_cmdq_V2(0x36, 1, &data, 1);
			lcm_set_gpio_output(GPIO_FAN_UD_SW, 1); 
            break;
        default:
            break;
    }
}

void lcm_rotate_horizontal_func(int mode)
{
    unsigned char data;
    switch(mode)
    {
        case 0:  //杩樺師鍒濆?嬬姸鎬?
            //data = 0x02;
            //dsi_set_cmdq_V2(0x36, 1, &data, 1);
            lcm_set_gpio_output(GPIO_FAN_RL_SW, 0);
            break;
        case 1: //姘村钩缈昏浆
            //data = 0x07;
            //dsi_set_cmdq_V2(0x36, 1, &data, 1);
            lcm_set_gpio_output(GPIO_FAN_RL_SW, 1);
            break;
        default:
            break;
    }

}

void lcm_rotate_180_horizontal_func(void)
{
    //unsigned char data = 0x00;
    
    //dsi_set_cmdq_V2(0x36, 1, &data, 1);
    lcm_set_gpio_output(GPIO_FAN_UD_SW, 1); 
    lcm_set_gpio_output(GPIO_FAN_RL_SW, 1);
}

void lcm_init_screen_transform(void)
{
    char* flip_value = get_env("flip_horizontal");
    int is_flip = 0;
    char* rotate_value = get_env("rotate_180");
    int is_rotate = 0;
	printf("lcm_init_screen_transform lk %s\r\n", flip_value);
	printf("lcm_init_screen_transform lk %s\r\n", rotate_value);
    if (flip_value != NULL && flip_value[0] == '1')
    {
        is_flip = 1;
    }

    if (rotate_value != NULL && rotate_value[0] == '1')
    {
        is_rotate = 1;
    }

     if (is_rotate == 1)
    {
        lcm_rotate_180_func(1);
    }
	else
	{
	    lcm_rotate_180_func(0);
	}
	
    if (is_flip == 1)
    {
        lcm_rotate_horizontal_func(1);
    }
	else
	{
        lcm_rotate_horizontal_func(0);	
	}
}




//extern void DSI_clk_HS_mode(unsigned char enter);
#define SPI_CE_LOW lcm_set_gpio_output(GPIO_SPI_CE,0)
#define SPI_CE_HIGH  lcm_set_gpio_output(GPIO_SPI_CE,1)
#define SPI_CLK_LOW  lcm_set_gpio_output(GPIO_SPI_CLK,0)
#define SPI_CLK_HIGH  lcm_set_gpio_output(GPIO_SPI_CLK,1)
#define SPI_MO_HIGH  lcm_set_gpio_output(GPIO_SPI_MISO,1)
#define SPI_MO_LOW  lcm_set_gpio_output(GPIO_SPI_MISO,0)
#define SPI_MI_HIGH  lcm_set_gpio_output(GPIO_SPI_MOSI,1)
#define SPI_MI_LOW  lcm_set_gpio_output(GPIO_SPI_MOSI,0)
#if 1
static void spi_write(u8 addr,u8 value)
{
	int i;
	//u16 data;
	//data = addr<<7|value;
	u8 data = addr;
	u8 data1 = value;
	SPI_CE_LOW;
	UDELAY(1);
	SPI_CLK_HIGH;
	SPI_MI_HIGH;
      	UDELAY(10);
	for(i=7;i>=0;i--) //for( i=0 ; i<16 ;++i)
	{
		SPI_CLK_LOW;
		UDELAY(10);
		 if(data & (1<<i))  //if(data & (1<<i))
		{
			SPI_MI_HIGH;
		}
		else
		{
			SPI_MI_LOW;
		}
		UDELAY(10);
		SPI_CLK_HIGH;
		UDELAY(10);		

		//data <<= 1;
	}
	for(i=7;i>=0;i--) //for( i=0 ; i<16 ;++i)
	{
		SPI_CLK_LOW;
		UDELAY(10);
		 if(data1 & (1<<i))  //if(data & (1<<i))
		{
			SPI_MI_HIGH;
		}
		else
		{
			SPI_MI_LOW;
		}
		UDELAY(10);
		SPI_CLK_HIGH;
		UDELAY(10);		

	}
	SPI_MI_HIGH;
	SPI_CE_HIGH;
}
#else
void SPI_TimeDelay(void)
{
int i =0;    
for(i = 0; i < 10; i++)
	{    }
}

void SPI_SendData(unsigned char data)
{   
	int i;   
	for(i=7;i>=0;i--)    
		{        
		gpio_direction_output(spi_clk,0);        
		SPI_TimeDelay();        
		if(data & (1<<i))
			gpio_direction_output(spi_out,1);       
		else            
			gpio_direction_output(spi_out,0);        
		gpio_direction_output(spi_clk,1);        
		SPI_TimeDelay();    
		}  
     //SPI_TimeDelay();
     }

void  LCD_spi_write_cmd_and_data(unsigned char cmd, unsigned char data)
{	
   gpio_direction_output(spi_cs,0);   
   SPI_SendData(cmd);   
   SPI_SendData(data);    
   gpio_direction_output(spi_cs,1);
}
#endif


static void gpio_set_status()
{
	mt_set_gpio_mode(GPIO_POWER_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_POWER_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_POWER_EN, GPIO_OUT_ONE);
	mdelay(100);
	mt_set_gpio_mode(LCM_EN_0, GPIO_MODE_00);
	mt_set_gpio_dir(LCM_EN_0, GPIO_DIR_OUT);
	mt_set_gpio_out(LCM_EN_0, GPIO_OUT_ONE);
	mdelay(300);
	mt_set_gpio_mode(GPIO_DCDC_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_DCDC_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_DCDC_EN, GPIO_OUT_ONE);
}








static void lcm_init(void)
{   

printf("lcm init csh ");
	//lcm_set_gpio_output(GPIO_LCM_PWR, GPIO_OUT_ZERO);   //enadble VCC_LCD 屏供电
	//MDELAY(20);
//	lcm_set_gpio_output(GPIO_LCM_PWR, GPIO_OUT_ONE);   //enadble VCC_LCD 屏供电

	if(kernel_power_off_charging_detection())
    {
            printf(" < Kernel Power Off Charging Detection Ok> \n");
            g_boot_mode = KERNEL_POWER_OFF_CHARGING_BOOT;
    }

	MDELAY(20);	
	gpio_set_status();
	// lcm_set_gpio_output(GPIO_LED_EN_SW,1);
	printk("===cxq==lcm_initt==222==%d===",mt_get_gpio_out(GPIO_LED_EN_SW));
	lcm_init_screen_transform();
	lcm_set_gpio_output(GPIO_LT8911B_PWR, GPIO_OUT_ONE);   //enadble LT8911B_PWR  8911电
	MDELAY(20);
#if  1
//		lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ZERO);		//屏背光使能 开始亮背光
#endif

	lcm_set_gpio_output(GPIO_LVDS_STBY, 1); 
	MDELAY(20);

	lcm_set_gpio_output(GPIO_LVDS_RST, 1); 
	MDELAY(50);
	lcm_set_gpio_output(GPIO_LVDS_RST, 0); 
	MDELAY(50);
	lcm_set_gpio_output(GPIO_LVDS_RST, 1); 
	MDELAY(50);

    /* DSI clock enter High Speed mode */
    DSI_clk_HS_mode(0, NULL, true);
	
	MDELAY(100);
	//init_lt8911_registers();
	init_tc358775_registers();
	//push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
	MDELAY(50);
	
	// if(kernel_power_off_charging_detection())
    // {
    //         printf(" < Kernel Power Off Charging Detection Ok> \n");
    //         g_boot_mode = KERNEL_POWER_OFF_CHARGING_BOOT;
    // }
    // else
    {
        printf("< Kernel Enter Normal Boot > \n");
        gpio_set_pwm(PWM1, 0xFFFFFFF); // 风扇PWM初始化为100%
        MDELAY(20);
		gpio_set_pwm(PWM2,  0x0FFFFFF);
		MDELAY(20);
        disp_pwm_set_backlight(DISP_PWM0,  900); 
       // lcm_set_gpio_output(GPIO_FAN_EN, 1); // 内部风扇
        MDELAY(20);
        lcm_set_gpio_output(GPIO_TEC_EN, 1);
    }
	
	
	//	lcm_set_gpio_output(GPIO_LVDS_LCD_VDD, GPIO_OUT_ONE);	 //enadble VCC_LCD 屏供电

	//lcm_set_gpio_output(GPIO_LCM_PWR, GPIO_OUT_ZERO);   //enadble VCC_LCD 屏供电
	//MDELAY(50);
	//lcm_set_gpio_output(GPIO_LCM_PWR, GPIO_OUT_ONE);   //enadble VCC_LCD 屏供电

	//lcm_set_gpio_output(GPIO_LCM_RST, 0);	
	//MDELAY(10);
	//lcm_set_gpio_output(GPIO_LCM_RST, 1);
	//MDELAY(20);	
#if  1
//	lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ONE); 		//屏背光使能 开始亮背光
#endif
	//MDELAY(150);

//	printf("LK %s enter", __func__);
}

static void lcm_suspend(void)
{	
#ifdef BUILD_LK
	printf("LK %s enter", __func__);
#else
	printk("KERNEL %s enter", __func__);
#endif

#if 0
	lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ZERO); 		// 背光
	MDELAY(10);
#endif
//	lcm_set_gpio_output(GPIO_LCM_PWR, GPIO_OUT_ZERO); 			//屏供电
/*	lcm_set_gpio_output(LCM_POWER_EN, GPIO_OUT_ZERO); 			//背光电源
	MDELAY(10);
	
	lcm_set_gpio_output(GPIO_LCM_RST, GPIO_OUT_ZERO);
	MDELAY(10);
*/
	lcm_set_gpio_output(GPIO_LVDS_STBY, 0); 
	MDELAY(10);
	lcm_set_gpio_output(GPIO_LVDS_RST, 0); 
	MDELAY(10);
	lcm_set_gpio_output(GPIO_LT8911B_PWR, GPIO_OUT_ZERO);	//lt8911b下电
	MDELAY(10);
#ifdef BUILD_LK
	printf("LK %s exit", __func__);
#else
	printk("KERNEL %s exit", __func__);
#endif

	
}

static void lcm_resume(void)
{
#ifdef BUILD_LK
	printf("LK %s enter", __func__);
#else
	printk("KERNEL %s enter\n", __func__);
#endif

	lcm_init();	

#ifdef BUILD_LK
	printf("LK %s exit", __func__);
#else
	printk("KERNEL %s exit\n", __func__);
#endif
 
}

LCM_DRIVER c106_tc358775_ltl106hl01_lcm_drv = 
{
   .name			= "c106_tc358775_ltl106hl01",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,

};
