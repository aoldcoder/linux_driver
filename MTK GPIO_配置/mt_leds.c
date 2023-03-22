/******************************************************************************
 * mt_leds.c
 * 
 * Copyright 2010 MediaTek Co.,Ltd.
 * 
 * DESCRIPTION:
 *
 ******************************************************************************/
#include <platform/mt_reg_base.h>
#include <platform/mt_typedefs.h>

#include <platform/mt_pwm.h>
#include <platform/mt_gpio.h>
#include <platform/mt_leds.h>

#include <platform/mt_pmic.h> 
#include <platform/upmu_common.h> 
#include <platform/mt_pmic_wrap_init.h>
#include <platform/mt_gpt.h>
#include <platform/boot_mode.h>

//yan add
#include <platform/mt_i2c.h>
//yan end

// #include <linux/printk.h>
#include <mt_disp_drv.h>

extern void mt_pwm_disable(U32 pwm_no, BOOL pmic_pad);
extern int strcmp(const char *cs, const char *ct);
extern void gpio_set_pwm(int pwm_num, u32 data);

/****************************************************************************
 * DEBUG MACROS
 ***************************************************************************/
int debug_enable = 1;
#define LEDS_DEBUG(format, args...) do{ \
		if(debug_enable) \
		{\
			dprintf(CRITICAL,format,##args);\
		}\
	}while(0)
#define LEDS_INFO LEDS_DEBUG 	
/****************************************************************************
 * structures
 ***************************************************************************/
static int g_lastlevel[MT65XX_LED_TYPE_TOTAL] = {-1, -1, -1, -1, -1, -1, -1};
int backlight_PWM_div = CLK_DIV1;

// Use Old Mode of PWM to suppoort 256 backlight level
#define BACKLIGHT_LEVEL_PWM_256_SUPPORT 256
extern unsigned int Cust_GetBacklightLevelSupport_byPWM(void);

/****************************************************************************
 * function prototypes
 ***************************************************************************/

/* internal functions */
static int brightness_set_pwm(int pwm_num, enum led_brightness level,struct PWM_config *config_data);
static int led_set_pwm(int pwm_num, enum led_brightness level);
static int brightness_set_pmic(enum mt65xx_led_pmic pmic_type, enum led_brightness level);
//static int brightness_set_gpio(int gpio_num, enum led_brightness level);
static int mt65xx_led_set_cust(struct cust_mt65xx_led *cust, int level);
/****************************************************************************
 * global variables
 ***************************************************************************/
static unsigned int limit = 255;

/****************************************************************************
 * global variables
 ***************************************************************************/

 //qxw add for  
extern void lcm_rotate_180_func(int mode);
extern void lcm_rotate_horizontal_func(int mode);
extern void lcm_rotate_180_horizontal_func(void);

extern void lcm_en(void);
//qxw end 

//qxw add
void lcm_init_transform(void)
{
		char* flip_value = get_env("flip_horizontal");
    int is_flip = 0;
    char* rotate_value = get_env("rotate_180");
    int is_rotate = 0;
    
    int value = 0;
                           
    if (flip_value != NULL && flip_value[0] == '1')
    {
        is_flip = 1;
    }

    if (rotate_value != NULL && rotate_value[0] == '1')
    {
        is_rotate = 1;
    }
    
    if(is_flip == 0 && is_rotate == 0)
    {
    		value = 0;    //??????    
    }	
    else if(is_flip == 1 && is_rotate == 0)
    {
        value = 1;   //???�M?
    }
    else if(is_flip == 0 && is_rotate == 1)
    {
        value = 2;   //??????
    }
    else if(is_flip == 1 && is_rotate == 1)
    {
        value = 3;   //???????
    }
    
    printf("qxw, value = %d\n",value);
        
    if(value == 0){
        lcm_rotate_180_func(0);
        lcm_rotate_horizontal_func(0);
    } else if(value == 1){   
    		lcm_rotate_180_func(0); 
    		lcm_rotate_horizontal_func(0);     		
        lcm_rotate_horizontal_func(1);                              		
    } else if(value == 2){
    		lcm_rotate_180_func(0);
    		lcm_rotate_horizontal_func(0);
        lcm_rotate_180_func(1);              
    } else if(value == 3){
				lcm_rotate_180_horizontal_func();
    }         
}

//qxw end
 
 

/****************************************************************************
 * internal functions
 ***************************************************************************/
static int brightness_mapto64(int level)
{
        if (level < 30)
                return (level >> 1) + 7;
        else if (level <= 120)
                return (level >> 2) + 14;
        else if (level <= 160)
                return level / 5 + 20;
        else
                return (level >> 3) + 33;
}
unsigned int brightness_mapping(unsigned int level)
{
	unsigned int mapped_level;
		
	mapped_level = level;
		   
	return mapped_level;
}

static int brightness_set_pwm(int pwm_num, enum led_brightness level,struct PWM_config *config_data)
{
	struct pwm_spec_config pwm_setting;
	unsigned int BacklightLevelSupport = Cust_GetBacklightLevelSupport_byPWM();
	pwm_setting.pwm_no = pwm_num;
	if (BacklightLevelSupport == BACKLIGHT_LEVEL_PWM_256_SUPPORT)
		pwm_setting.mode = PWM_MODE_OLD;
	else
		pwm_setting.mode = PWM_MODE_FIFO; // New mode fifo and periodical mode

	pwm_setting.pmic_pad = config_data->pmic_pad;
	if(config_data->div)
	{
		pwm_setting.clk_div = config_data->div;
		backlight_PWM_div = config_data->div;
	}
   else
     pwm_setting.clk_div = CLK_DIV1;
   
	if(BacklightLevelSupport== BACKLIGHT_LEVEL_PWM_256_SUPPORT)
	{
		if(config_data->clock_source)
		{
			pwm_setting.clk_src = PWM_CLK_OLD_MODE_BLOCK;
		}
		else
		{
			pwm_setting.clk_src = PWM_CLK_OLD_MODE_32K; // actually. it's block/1625 = 26M/1625 = 16KHz @ MT6571
		}

		pwm_setting.pwm_mode.PWM_MODE_OLD_REGS.IDLE_VALUE = 0;
		pwm_setting.pwm_mode.PWM_MODE_OLD_REGS.GUARD_VALUE = 0;
		pwm_setting.pwm_mode.PWM_MODE_OLD_REGS.GDURATION = 0;
		pwm_setting.pwm_mode.PWM_MODE_OLD_REGS.WAVE_NUM = 0;
		pwm_setting.pwm_mode.PWM_MODE_OLD_REGS.DATA_WIDTH = 255; // 256 level
		pwm_setting.pwm_mode.PWM_MODE_OLD_REGS.THRESH = level;

		LEDS_DEBUG("[LEDS][%d] LK: backlight_set_pwm:duty is %d/%d\n", BacklightLevelSupport, level, pwm_setting.pwm_mode.PWM_MODE_OLD_REGS.DATA_WIDTH);
		LEDS_DEBUG("[LEDS][%d] LK: backlight_set_pwm:clk_src/div is %d%d\n", BacklightLevelSupport, pwm_setting.clk_src, pwm_setting.clk_div);
		if(level >0 && level < 256)
		{
			pwm_set_spec_config(&pwm_setting);
			LEDS_DEBUG("[LEDS][%d] LK: backlight_set_pwm: old mode: thres/data_width is %d/%d\n", BacklightLevelSupport, pwm_setting.pwm_mode.PWM_MODE_OLD_REGS.THRESH, pwm_setting.pwm_mode.PWM_MODE_OLD_REGS.DATA_WIDTH);
		}
		else
		{
			LEDS_DEBUG("[LEDS][%d] LK: Error level in backlight\n", BacklightLevelSupport);
			mt_pwm_disable(pwm_setting.pwm_no, config_data->pmic_pad);
		}
		return 0;

	}
	else
	{
		if(config_data->clock_source)
		{
			pwm_setting.clk_src = PWM_CLK_NEW_MODE_BLOCK;
		}
		else
		{
			pwm_setting.clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
		}

		if(config_data->High_duration && config_data->low_duration)
		{
			pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.HDURATION = config_data->High_duration;
			pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.LDURATION = pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.HDURATION;
		}
		else
		{
			pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.HDURATION = 4;
			pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.LDURATION = 4;
		}

		pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.IDLE_VALUE = 0;
		pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.GUARD_VALUE = 0;
		pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 31;
		pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.GDURATION = (pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.HDURATION + 1) * 32 - 1;
		pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;

		LEDS_DEBUG("[LEDS] LK: backlight_set_pwm:duty is %d\n", level);
		LEDS_DEBUG("[LEDS] LK: backlight_set_pwm:clk_src/div/high/low is %d%d%d%d\n", pwm_setting.clk_src, pwm_setting.clk_div, pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.HDURATION, pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.LDURATION);

		if(level > 0 && level <= 32)
		{
			pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.GUARD_VALUE = 0;
			pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.SEND_DATA0 = (1 << level) - 1;
			pwm_set_spec_config(&pwm_setting);
		}else if(level > 32 && level <= 64)
		{
			pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.GUARD_VALUE = 1;
			level -= 32;
			pwm_setting.pwm_mode.PWM_MODE_FIFO_REGS.SEND_DATA0 = (1 << level) - 1 ;
			pwm_set_spec_config(&pwm_setting);
		}else
		{
			LEDS_DEBUG("[LEDS] LK: Error level in backlight\n");
			mt_pwm_disable(pwm_setting.pwm_no, config_data->pmic_pad);
		}

		return 0;
	}
}

static int led_set_pwm(int pwm_num, enum led_brightness level)
{
	struct pwm_spec_config pwm_setting;
	pwm_setting.pwm_no = pwm_num;
	pwm_setting.clk_div = CLK_DIV1; 		

	pwm_setting.pwm_mode.PWM_MODE_OLD_REGS.DATA_WIDTH = 50;
    
	// We won't choose 32K to be the clock src of old mode because of system performance.
	// The setting here will be clock src = 26MHz, CLKSEL = 26M/1625 (i.e. 16K)
		pwm_setting.clk_src = PWM_CLK_OLD_MODE_32K;
    
	if(level)
	{
		pwm_setting.pwm_mode.PWM_MODE_OLD_REGS.THRESH = 15;
	}else
	{
		pwm_setting.pwm_mode.PWM_MODE_OLD_REGS.THRESH = 0;
	}
	LEDS_INFO("[LEDS]LK: brightness_set_pwm: level=%d, clk=%d \n\r", level, pwm_setting.clk_src);
	pwm_set_spec_config(&pwm_setting);
	
	return 0;
}

static int brightness_set_pmic(enum mt65xx_led_pmic pmic_type, enum led_brightness level)
{
		int tmp_level = level;	
		//static bool backlight_init_flag[4] = {false, false, false, false};
		static bool backlight_init_flag = false;
		//static bool led_init_flag[4] = {false, false, false, false};	
		//static bool first_time = true;
	
		static unsigned char duty_mapping[108] = {
        0,	0,	0,	0,	0,	6,	1,	2,	1,	10,	1,	12,
        6,	14,	2,	3,	16,	2,	18,	3,	6,	10,	22, 3,
        12,	8,	6,	28,	4,	30,	7,	10,	16,	6,	5,	18,
        12,	7,	6,	10,	8,	22,	7,	9,	16,	12,	8,	10,
        13,	18,	28,	9,	30,	20,	15,	12,	10,	16,	22,	13,
        11,	14,	18,	12,	19,	15,	26,	13,	16,	28,	21,	14,
        22,	30,	18,	15,	19,	16,	25,	20,	17,	21,	27,	18,
        22,	28,	19,	30,	24,	20,	31,	25,	21,	26,	22,	27,
        23,	28,	24,	30,	25,	31,	26,	27,	28,	29,	30,	31,
    	};
    	static unsigned char current_mapping[108] = {
        1,	2,	3,	4,	5,	0,	3,	2,	4,	0,	5,	0,
        1,	0,	4,	3,	0,	5,	0,	4,	2,	1,	0,	5,
        1,	2,	3,	0,	5,	0,	3,	2,	1,	4,	5,	1,
        2,	4,	5,	3,	4,	1,	5,	4,	2,	3,	5,	4,
        3,	2,	1,	5,	1,	2,	3,	4,	5,	3,	2,	4,
        5,	4,	3,	5,	3,	4,	2,	5,	4,	2,	3,	5,
        3,	2,	4,	5,	4,	5,	3,	4,	5,	4,	3,	5,
        4,	3,	5,	3,	4,	5,	3,	4,	5,	4,	5,	4,
        5,	4,	5,	4,	5,	4,	5,	5,	5,	5,	5,	5,
    	};

	LEDS_INFO("[LEDS]LK: PMIC Type: %d, Level: %d\n", pmic_type, level);

	if (pmic_type == MT65XX_LED_PMIC_LCD_ISINK)
	{
		if(backlight_init_flag == false)
		{
             //mt6325_upmu_set_rg_g_drv_2m_ck_pdn(0x0); // Disable power down 
            // For backlight: Current: 24mA, PWM frequency: 20K, Duty: 20~100, Soft start: off, Phase shift: on
            // ISINK0
            //mt6325_upmu_set_rg_drv_isink0_ck_pdn(0x0); // Disable power down    
            //mt6325_upmu_set_rg_drv_isink0_ck_cksel(0x1); // Freq = 1Mhz for Backlight
			//mt6325_upmu_set_isink_ch0_mode(ISINK_PWM_MODE);
            //mt6325_upmu_set_isink_ch0_step(ISINK_5); // 24mA
            //mt6325_upmu_set_isink_sfstr0_en(0x0); // Disable soft start
			//mt6325_upmu_set_rg_isink0_double_en(0x1); // Enable double current
			//mt6325_upmu_set_isink_phase_dly_tc(0x0); // TC = 0.5us
			//mt6325_upmu_set_isink_phase0_dly_en(0x1); // Enable phase delay
            //mt6325_upmu_set_isink_chop0_en(0x1); // Enable CHOP clk
            // ISINK1
            //mt6325_upmu_set_rg_drv_isink1_ck_pdn(0x0); // Disable power down   
            //mt6325_upmu_set_rg_drv_isink1_ck_cksel(0x1); // Freq = 1Mhz for Backlight
			//mt6325_upmu_set_isink_ch1_mode(ISINK_PWM_MODE);
            //mt6325_upmu_set_isink_ch1_step(ISINK_3); // 24mA
            //mt6325_upmu_set_isink_sfstr1_en(0x0); // Disable soft start
			//mt6325_upmu_set_rg_isink1_double_en(0x1); // Enable double current
			//mt6325_upmu_set_isink_phase1_dly_en(0x1); // Enable phase delay
            //mt6325_upmu_set_isink_chop1_en(0x1); // Enable CHOP clk         
            // ISINK2
            //mt6325_upmu_set_rg_drv_isink2_ck_pdn(0x0); // Disable power down   
            //mt6325_upmu_set_rg_drv_isink2_ck_cksel(0x1); // Freq = 1Mhz for Backlight
			//mt6325_upmu_set_isink_ch2_mode(ISINK_PWM_MODE);
            //mt6325_upmu_set_isink_ch2_step(ISINK_3); // 24mA
            //mt6325_upmu_set_isink_sfstr2_en(0x0); // Disable soft start
			//mt6325_upmu_set_rg_isink2_double_en(0x1); // Enable double current
			//mt6325_upmu_set_isink_phase2_dly_en(0x1); // Enable phase delay
            //mt6325_upmu_set_isink_chop2_en(0x1); // Enable CHOP clk   
            // ISINK3
         	//mt6325_upmu_set_rg_drv_isink3_ck_pdn(0x0); // Disable power down   
            //mt6325_upmu_set_rg_drv_isink3_ck_cksel(0x1); // Freq = 1Mhz for Backlight
			//mt6325_upmu_set_isink_ch3_mode(ISINK_PWM_MODE);
            //mt6325_upmu_set_isink_ch3_step(ISINK_3); // 24mA
            //mt6325_upmu_set_isink_sfstr3_en(0x0); // Disable soft start
			//mt6325_upmu_set_rg_isink3_double_en(0x1); // Enable double current
			//mt6325_upmu_set_isink_phase3_dly_en(0x1); // Enable phase delay
            //mt6325_upmu_set_isink_chop3_en(0x1); // Enable CHOP clk              
			backlight_init_flag = true;
		}
		
		if (level) 
		{
			level = brightness_mapping(tmp_level);
			if(level == ERROR_BL_LEVEL)
				level = limit;    
            if(level == limit)
            {
                level = 108;
            }
            else
            {
                level =((level * 108) / limit) + 1;
            }
            LEDS_INFO("[LEDS]LK: Level Mapping = %d \n", level);
            LEDS_INFO("[LEDS]LK: ISINK DIM Duty = %d \n", duty_mapping[level-1]);
            LEDS_INFO("[LEDS]LK: ISINK Current = %d \n", current_mapping[level-1]);
            //mt6325_upmu_set_isink_dim0_duty(duty_mapping[level-1]);
            //mt6325_upmu_set_isink_dim1_duty(duty_mapping[level-1]);
            //mt6325_upmu_set_isink_dim2_duty(duty_mapping[level-1]);
            //mt6325_upmu_set_isink_dim3_duty(duty_mapping[level-1]);
            //mt6325_upmu_set_isink_ch0_step(current_mapping[level-1]);
            //mt6325_upmu_set_isink_ch1_step(current_mapping[level-1]);
            //mt6325_upmu_set_isink_ch2_step(current_mapping[level-1]);
            //mt6325_upmu_set_isink_ch3_step(current_mapping[level-1]);
            //mt6325_upmu_set_isink_dim0_fsel(ISINK_2M_20KHZ); // 20Khz
            //mt6325_upmu_set_isink_dim1_fsel(ISINK_2M_20KHZ); // 20Khz
            //mt6325_upmu_set_isink_dim2_fsel(ISINK_2M_20KHZ); // 20Khz
            //mt6325_upmu_set_isink_dim3_fsel(ISINK_2M_20KHZ); // 20Khz            
            //mt6325_upmu_set_isink_ch0_en(0x1); // Turn on ISINK Channel 0
            //mt6325_upmu_set_isink_ch1_en(0x1); // Turn on ISINK Channel 1
            //mt6325_upmu_set_isink_ch2_en(0x1); // Turn on ISINK Channel 2
            //mt6325_upmu_set_isink_ch3_en(0x1); // Turn on ISINK Channel 3
		}
		else 
		{
            //mt6325_upmu_set_isink_ch0_en(0x0); // Turn off ISINK Channel 0
            //mt6325_upmu_set_isink_ch1_en(0x0); // Turn off ISINK Channel 1
            //mt6325_upmu_set_isink_ch2_en(0x0); // Turn off ISINK Channel 2
            //mt6325_upmu_set_isink_ch3_en(0x0); // Turn off ISINK Channel 3
		}
        
		return 0;
	}
	else if(pmic_type == MT65XX_LED_PMIC_NLED_ISINK0)
	{
		//mt6325_upmu_set_rg_drv_32k_ck_pdn(0x0); // Disable power down  
		//mt6325_upmu_set_rg_drv_isink0_ck_pdn(0);
    	//mt6325_upmu_set_rg_drv_isink0_ck_cksel(0);
		//mt6325_upmu_set_isink_ch0_mode(ISINK_PWM_MODE);
	    //mt6325_upmu_set_isink_ch0_step(ISINK_3);//16mA
    	//mt6325_upmu_set_isink_dim0_duty(15);
		//mt6325_upmu_set_isink_dim0_fsel(ISINK_1KHZ);//1KHz
		if (level) 
		{
            //mt6325_upmu_set_rg_drv_32k_ck_pdn(0x0); // Disable power down            
            //mt6325_upmu_set_isink_ch0_en(0x1); // Turn on ISINK Channel 0
			
		}
		else 
		{
            //mt6325_upmu_set_isink_ch0_en(0x0); // Turn off ISINK Channel 0
		}
		return 0;
	}
	else if(pmic_type == MT65XX_LED_PMIC_NLED_ISINK1)
	{
        //mt6325_upmu_set_rg_drv_32k_ck_pdn(0x0); // Disable power down  
		//mt6325_upmu_set_rg_drv_isink1_ck_pdn(0);
    	//mt6325_upmu_set_rg_drv_isink1_ck_cksel(0);
		//mt6325_upmu_set_isink_ch1_mode(ISINK_PWM_MODE);
	    //mt6325_upmu_set_isink_ch1_step(ISINK_3);//16mA
    	//mt6325_upmu_set_isink_dim1_duty(15);
		//mt6325_upmu_set_isink_dim1_fsel(ISINK_1KHZ);//1KHz
		if (level) 
		{
            //mt6325_upmu_set_rg_drv_32k_ck_pdn(0x0); // Disable power down            
            //mt6325_upmu_set_isink_ch1_en(0x1); // Turn on ISINK Channel 0
			
		}
		else 
		{
            //mt6325_upmu_set_isink_ch1_en(0x0); // Turn off ISINK Channel 0
		}
		return 0;
	}
	else if(pmic_type == MT65XX_LED_PMIC_NLED_ISINK2)
	{
		#if 0
        //mt6325_upmu_set_rg_drv_32k_ck_pdn(0x0); // Disable power down  
		//mt6325_upmu_set_rg_drv_isink2_ck_pdn(0);
    	//mt6325_upmu_set_rg_drv_isink2_ck_cksel(0);
		//mt6325_upmu_set_isink_ch2_mode(ISINK_PWM_MODE);
	    //mt6325_upmu_set_isink_ch2_step(ISINK_3);//16mA
    	//mt6325_upmu_set_isink_dim2_duty(15);
		//mt6325_upmu_set_isink_dim2_fsel(ISINK_1KHZ);//1KHz
		if (level) 
		{
            //mt6325_upmu_set_rg_drv_32k_ck_pdn(0x0); // Disable power down            
            //mt6325_upmu_set_isink_ch2_en(0x1); // Turn on ISINK Channel 0
			
		}
		else 
		{
            //mt6325_upmu_set_isink_ch2_en(0x0); // Turn off ISINK Channel 0
		}
		#endif
	
            upmu_set_rg_isink2_ck_pdn(0x0); // Disable power down    
            upmu_set_rg_isink2_ck_sel(0x0); // Freq = 32KHz for Indicator            
            upmu_set_isink_dim2_duty(15); // 16 / 32, no use for register mode
            upmu_set_isink_dim2_fsel(0x0); // 1KHz, no use for register mode
            upmu_set_isink_ch0_mode(0x0);
            upmu_set_isink_ch2_step(0x3); // 16mA
            upmu_set_isink_chop2_en(0x0); // Disable CHOP clk
		
		if (level) 
		{
            //upmu_set_rg_drv_2m_ck_pdn(0x0); // Disable power down (indicator no need?)     
            upmu_set_rg_drv_32k_ck_pdn(0x0); // Disable power down            
            upmu_set_isink_ch2_en(0x1); // Turn on ISINK Channel 2
			
		}
		else 
		{
            upmu_set_isink_ch2_en(0x0); // Turn off ISINK Channel 2
		}
		
		
		return 0;
	}
    else if(pmic_type == MT65XX_LED_PMIC_NLED_ISINK3)
	{
		#if 0
        //mt6325_upmu_set_rg_drv_32k_ck_pdn(0x0); // Disable power down  
		//mt6325_upmu_set_rg_drv_isink3_ck_pdn(0);
    	//mt6325_upmu_set_rg_drv_isink3_ck_cksel(0);
		//mt6325_upmu_set_isink_ch3_mode(ISINK_PWM_MODE);
	    //mt6325_upmu_set_isink_ch3_step(ISINK_3);//16mA
    	//mt6325_upmu_set_isink_dim3_duty(15);
		//mt6325_upmu_set_isink_dim3_fsel(ISINK_1KHZ);//1KHz
		if (level) 
		{
            //mt6325_upmu_set_rg_drv_32k_ck_pdn(0x0); // Disable power down            
            //mt6325_upmu_set_isink_ch3_en(0x1); // Turn on ISINK Channel 0
			
		}
		else 
		{
            //mt6325_upmu_set_isink_ch3_en(0x0); // Turn off ISINK Channel 0
		}
		#endif
		
		  upmu_set_rg_isink3_ck_pdn(0x0); // Disable power down    
            upmu_set_rg_isink3_ck_sel(0x0); // Freq = 32KHz for Indicator            
            upmu_set_isink_dim3_duty(15); // 16 / 32, no use for register mode
            upmu_set_isink_dim3_fsel(0x0); // 1KHz, no use for register mode
            upmu_set_isink_ch0_mode(0x0);
            upmu_set_isink_ch3_step(0x3); // 16mA
            upmu_set_isink_chop3_en(0x0); // Disable CHOP clk

		if (level) 
		{
           // upmu_set_rg_drv_2m_ck_pdn(0x0); // Disable power down (indicator no need?)     
            upmu_set_rg_drv_32k_ck_pdn(0x0); // Disable power down            
            upmu_set_isink_ch3_en(0x1); // Turn on ISINK Channel 3
			
		}
		else 
		{
            upmu_set_isink_ch3_en(0x0); // Turn off ISINK Channel 3
		}
		return 0;
	}

	return -1;
}
#ifdef IWLED_SUPPORT
static void mt_vmled_init(void)
{
	unsigned int rdata;
	mt6332_upmu_set_rg_vwled_32k_ck_pdn(0x0);
	mt6332_upmu_set_rg_vwled_6m_ck_pdn(0x0);
	mt6332_upmu_set_rg_vwled_1m_ck_pdn(0x0);
	mt6332_upmu_set_rg_vwled_rst(0x1);
	mt6332_upmu_set_rg_vwled_rst(0x0);
	/*STRUP_CON14=0x0*/
	mt6332_upmu_set_rg_en_smt(0);
	mt6332_upmu_set_rg_en_sr(0);
	mt6332_upmu_set_rg_en_e8(0);
	mt6332_upmu_set_rg_en_e4(0);
	mt6332_upmu_set_rg_testmode_swen(0);
	mt6332_upmu_set_rg_strup_rsv(0);
	/*IWLED_CON0=0x0AE8*/
	mt6332_upmu_set_rg_iwled_frq_count(0x0AE8); //50
	/*CH Turn On*/
	mt6332_upmu_set_rg_iwled0_status(1);
	mt6332_upmu_set_rg_iwled1_status(1);
	/*IWLED_CON1=0x3D00*/
	mt6332_upmu_set_rg_iwled_cs(3);
	mt6332_upmu_set_rg_iwled_slp(3);
	mt6332_upmu_set_rg_iwled_rc(1);
	/*IWLED_DEG=0XE000*/
	mt6332_upmu_set_rg_iwled_slp_deg_en(0);
	mt6332_upmu_set_rg_iwled_ovp_deg_en(1);
	mt6332_upmu_set_rg_iwled_oc_deg_en(0);
	/*IWLED_CON4=0X8000*/
	mt6332_upmu_set_rg_iwled_rsv(0x8);

	mdelay(100);

	/*IWLED Channel enable*/
	mt6332_upmu_set_rg_iwled0_en(0x1);
	mt6332_upmu_set_rg_iwled1_en(0x1);

	/*dump RG*/
	pwrap_read(0x8C20, &rdata);
	LEDS_DEBUG("0x8C20=0x%x\n",rdata);
	pwrap_read(0x8094, &rdata);
	LEDS_DEBUG("0x8094=0x%x\n",rdata);
	pwrap_read(0x809A, &rdata);
	LEDS_DEBUG("0x809A=0x%x\n",rdata);
	pwrap_read(0x8CD4, &rdata);
	LEDS_DEBUG("0x8CD4=0x%x\n",rdata);
	pwrap_read(0x8C08, &rdata);
	LEDS_DEBUG("0x8C08=0x%x\n",rdata);
	pwrap_read(0x8CD8, &rdata);
	LEDS_DEBUG("0x8CD8=0x%x\n",rdata);
	pwrap_read(0x8CDC, &rdata);
	LEDS_DEBUG("0x8CDC=0x%x\n",rdata);
	pwrap_read(0x8CDA, &rdata);
	LEDS_DEBUG("0x8CDA=0x%x\n",rdata);
	pwrap_read(0x8CD6, &rdata);
	LEDS_DEBUG("0x8CD6=0x%x\n",rdata);
	pwrap_read(0x8CE6, &rdata);
	LEDS_DEBUG("0x8CE6=0x%x\n",rdata);
}
#endif


//yan
static int _atoi(const char *str) {
    int ret = 0, i = 0;
    bool neg = false;
    
    //find neg and skip space
    for (i = 0; i < strlen(str); i++) {
        if ((str[i] >= '0' && str[i] <= '9')
            || str[i] == ' ' || str[i] == '-') {
            if (str[i] != ' ') {
                if (str[i] == '-') {
                    neg = true;
                    i++;
                }
                break;
            }
        } else {
            //error
            return 0;
        }
    }
    
    for (; i < strlen(str); i++) {
        int value = str[i];
        
        if (value < '0' || value > '9') {
            break;
        }
        ret = ret * 10 + (value - '0');
    }
    return neg ? -ret : ret;
}
//yan end

//yan init for dpp3430
#ifdef DPP3430_SUPPORT

//#define GPIO_PAD2005_PROJON           (GPIO76 | 0x80000000)
//#define GPIO_3430_PARKZ           (GPIO74 | 0x80000000)

#define DPP3430_I2C_CH                      I2C0
#define DPP3430_I2C_ADDR                 (0x36 >> 1)

int dpp3430_i2c_write_8bit(u8 addr, u8 data)
{
    u8 buffer[2];
    U32 ret_code = 0;

    static struct mt_i2c_t i2c;
    i2c.id = DPP3430_I2C_CH;
    i2c.addr = DPP3430_I2C_ADDR;	/* i2c API will shift-left this value to 0xc0 */
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    buffer[0] = addr;
    buffer[1] = data;
    ret_code = i2c_write(&i2c, buffer, 2); // 0:I2C_PATH_NORMAL
    if (ret_code != 0)
    {
        return ret_code;
    }
    return 0;
}

int dpp3430_i2c_write_variably(u8 addr, u8 len, u8* data)
{
    u8 buffer[32];
    u8 i = 0;
    U32 ret_code = 0;
    static struct mt_i2c_t i2c;
    
    i2c.id = DPP3430_I2C_CH;
    i2c.addr = DPP3430_I2C_ADDR;	/* i2c API will shift-left this value to 0xc0 */
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    if (len >= 32)
    {
        return 0;
    }
    
    buffer[0] = addr;
    for (i = 0; i < len; i++)
    {
        buffer[i + 1] = data[i];
    }

    ret_code = i2c_write(&i2c, buffer, (len + 1)); // 0:I2C_PATH_NORMAL
    if (ret_code != 0)
    {
        return ret_code;
    }
    return 0;
}

u32 dpp3430_i2c_read(u8 addr, u8 len, u8* out_data)
{
    u8 buffer[1] = {0};
    u32 ret_code = 0;
    u32 data;

    static struct mt_i2c_t i2c;
    i2c.id = DPP3430_I2C_CH;
    i2c.addr = DPP3430_I2C_ADDR;	/* i2c API will shift-left this value to 0xc0 */
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    buffer[0] = addr;
    ret_code = i2c_write(&i2c, buffer, 1);    // set register command
    if (ret_code != 0)
        return ret_code;

    memset(out_data, 0, sizeof(u8) * len);
    
    ret_code = i2c_read(&i2c, out_data, len); 

    return ret_code;
}

int dpp3430_i2c_read_8bit(u8 addr)
{
    u8 buffer[1] = {0};
    u32 ret_code = 0;
    u32 data;

    static struct mt_i2c_t i2c;
    i2c.id = DPP3430_I2C_CH;
    i2c.addr = DPP3430_I2C_ADDR;	/* i2c API will shift-left this value to 0xc0 */
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    buffer[0] = addr;
    ret_code = i2c_write(&i2c, buffer, 1);    // set register command
    if (ret_code != 0)
        return -1;

    //memset(buffer, 0, sizeof(buffer));
    
    ret_code = i2c_read(&i2c, buffer, 1); 
    if (ret_code != 0)
        return -1;

    return buffer[0];
}

void dpp3430_i2c_freeze_screen(bool freeze)
{
    if (!freeze)
    {
        mdelay(30);
    }
    dpp3430_i2c_write_8bit(0x1A, freeze ? 1 : 0);
    if (freeze)
    {
        mdelay(30);
    }
}

void dpp3430_i2c_init_duty_ratio(void)
{
    //TODO
   // dpp3430_i2c_write_8bit(0x22, 0x04); 
}
//yan end

//yan screen transform
void dpp3430_i2c_init_screen_transform(void)
{
    char* flip_value = get_env("flip_horizontal");
    int is_flip = 0;
    char* rotate_value = get_env("rotate_180");
    int is_rotate = 0;

    if (flip_value != NULL && flip_value[0] == '1')
    {
        is_flip = 1;
    }

    if (rotate_value != NULL && rotate_value[0] == '1')
    {
        is_rotate = 1;
    }

    //printf("[yan test] rotate: %d, flip: %d\n", is_rotate, is_flip);
    
    if (is_flip == 1 && is_rotate == 1)
    {
        #ifdef DPP3430_UP_SIDE_DOWN_SUPPORT
        dpp3430_i2c_freeze_screen(true);
        dpp3430_i2c_write_8bit(0x14, 0x0);
        dpp3430_i2c_freeze_screen(false);
        #else
        dpp3430_i2c_freeze_screen(true);
        dpp3430_i2c_write_8bit(0x14, 0x06);
        dpp3430_i2c_freeze_screen(false);
        #endif
    }
    else if (is_rotate == 1)
    {
        #ifdef DPP3430_UP_SIDE_DOWN_SUPPORT
        dpp3430_i2c_freeze_screen(true);
        dpp3430_i2c_write_8bit(0x14, 0x02);
        dpp3430_i2c_freeze_screen(false);
        #else
        dpp3430_i2c_freeze_screen(true);
        dpp3430_i2c_write_8bit(0x14, 0x06);
        dpp3430_i2c_freeze_screen(false);
        #endif
    }
    else if (is_flip == 1)
    {
        #ifdef DPP3430_UP_SIDE_DOWN_SUPPORT
        dpp3430_i2c_freeze_screen(true);
        dpp3430_i2c_write_8bit(0x14, 0x06);
        dpp3430_i2c_freeze_screen(false);
        #else
        dpp3430_i2c_freeze_screen(true);
        dpp3430_i2c_write_8bit(0x14, 0x04);
        dpp3430_i2c_freeze_screen(false);
        #endif
    }
    else //normal mode, default rotate 180
    {
        #ifdef DPP3430_UP_SIDE_DOWN_SUPPORT
        dpp3430_i2c_freeze_screen(true);
        dpp3430_i2c_write_8bit(0x14, 0x04);
        dpp3430_i2c_freeze_screen(false);
        #endif
    }
}
//yan end

//yan keystone correction
void dpp3430_i2c_init_keystone_correction(void)
{
    char* mode = get_env("keystone_mode");
    char* value = get_env("keystone_manual_value");
    
    if (mode != NULL) {
        int modeInt = _atoi(mode);
        if (modeInt == 2 && value != NULL) { //manual
            int valueInt = _atoi(value);
            
            if (valueInt != 0) {
                u8 values[5] = {0};
                u16 temp = 0;
                
                dpp3430_i2c_freeze_screen(true);
                
                values[0] = 1;
                
                temp = 10; //throw_ratio
                values[1] = (u8) (temp & 0x00FF);
                values[2] = (u8) ((temp & 0xFF00) >> 8);
                temp = 10; //dmd_offset;
                values[3] = (u8) (temp & 0x00FF);
                values[4] = (u8) ((temp & 0xFF00) >> 8);
                dpp3430_i2c_write_variably(0x88, 5, values);
                
                memset(values, 0, sizeof(values));
                temp = (u16) valueInt; //angle;
                values[0] = (u8) (temp & 0x00FF);
                values[1] = (u8) ((temp & 0xFF00) >> 8);
                dpp3430_i2c_write_variably(0xbb, 2, values);
                
                dpp3430_i2c_freeze_screen(false);
            }
        }
    }
}
//yan end

int dpp3430_rgb_current_is_readed = 0;
int DPP3430_LED_R_CURRENT_DEFAULT = 0x1A4;//(0x01 << 8) | 0x95;
int DPP3430_LED_G_CURRENT_DEFAULT = 0x1A4;//(0x01 << 8) | 0x95;
int DPP3430_LED_B_CURRENT_DEFAULT = 0x1A4;//(0x01 << 8) | 0x95;

void dpp3430_i2c_set_brightness(int pwm_level)
{
    int percent = 0;
    int MIN_PERCENT = 10;
    int MAX_PERCENT = 92;
    u8 rgb_current[6] = {0};
    int current_r = 0, current_g = 0, current_b = 0;

    percent = (pwm_level - 40) * 100 / (1023 - 40);
    percent = (MAX_PERCENT - MIN_PERCENT) * percent / 100 + MIN_PERCENT;

    if (percent < MIN_PERCENT)
    {
        percent = MIN_PERCENT;
    }

    if (dpp3430_rgb_current_is_readed == 0)
    {
        memset(rgb_current, 0, sizeof(rgb_current));
        dpp3430_i2c_read(0x55, 6, rgb_current);
        
        //DPP3430_LED_R_CURRENT_DEFAULT = (rgb_current[1] << 8) | rgb_current[0];
        //DPP3430_LED_G_CURRENT_DEFAULT = (rgb_current[3] << 8) | rgb_current[2];
        //DPP3430_LED_B_CURRENT_DEFAULT = (rgb_current[5] << 8) | rgb_current[4];
        
        dpp3430_rgb_current_is_readed = 1;
    }

    memset(rgb_current, 0, sizeof(rgb_current));
    current_r = DPP3430_LED_R_CURRENT_DEFAULT * percent / 100;
    current_g = DPP3430_LED_G_CURRENT_DEFAULT * percent / 100;
    current_b = DPP3430_LED_B_CURRENT_DEFAULT * percent / 100;
    
    rgb_current[0] = (u8) (current_r & 0x00FF);
    rgb_current[1] = (u8) ((current_r & 0xFF00) >> 8);
    
    rgb_current[2] = (u8) (current_g & 0x00FF);
    rgb_current[3] = (u8) ((current_g & 0xFF00) >> 8);
    
    rgb_current[4] = (u8) (current_b & 0x00FF);
    rgb_current[5] = (u8) ((current_b & 0xFF00) >> 8);
    
    dpp3430_i2c_write_variably(0x54, 6, rgb_current);
}
#endif


//qxw add start
#define GPIO_DC_DET    (GPIO88 | 0x80000000)
//add end


#define GPIO_POWER_EN	(GPIO30	| 0x80000000)
#define GPIO_PROJ_EN	(GPIO32	| 0x80000000)
#define GPIO_DCDC_EN	(GPIO86	| 0x80000000)


//yan bootup status for IR
#define GPIO_29   (GPIO29 | 0x80000000)
//yan end
#define GPIO_49   (GPIO49 | 0x80000000)
//leung add
extern BOOTMODE g_boot_mode;
#if defined(DPP2607_SUPPORT)

#define GPIO_PAD1000_RST         (GPIO31 | 0x80000000)
#define GPIO_PAD1000_PROJON      (GPIO30 | 0x80000000)
#define GPIO_2607_INTF           (GPIO29 | 0x80000000)
#define GPIO_2607_LED_EN         (GPIO28 | 0x80000000)
#define GPIO_2607_PARKZ          (GPIO23 | 0x80000000)

#define DPP2607_I2C_CH                      I2C0
#define DPP2607_I2C_ADDR                 (0x36 >> 1)

int dpp2607_i2c_write(u8 addr, u32 data)
{
    u8 buffer[5];
    U32 ret_code = 0;

    static struct mt_i2c_t i2c;
    i2c.id = DPP2607_I2C_CH;
    i2c.addr = DPP2607_I2C_ADDR;	/* i2c API will shift-left this value to 0xc0 */
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    buffer[0] = addr;
    buffer[1] = (data & 0xFF000000) >> 24;
    buffer[2] = (data & 0x00FF0000) >> 16;
    buffer[3] = (data & 0x0000FF00) >> 8;
    buffer[4] = (u8) (data & 0x000000FF);

    ret_code = i2c_write(&i2c, buffer, 5); // 0:I2C_PATH_NORMAL
    if (ret_code != 0)
    {
        return ret_code;
    }
    return 0;
}

int dpp2607_i2c_write_8bit(u8 addr, u8 data)
{
    u8 buffer[2];
    U32 ret_code = 0;

    static struct mt_i2c_t i2c;
    i2c.id = DPP2607_I2C_CH;
    i2c.addr = DPP2607_I2C_ADDR;	/* i2c API will shift-left this value to 0xc0 */
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    buffer[0] = addr;
    buffer[1] = data;

    ret_code = i2c_write(&i2c, buffer, 2); // 0:I2C_PATH_NORMAL
    if (ret_code != 0)
    {
        return ret_code;
    }
    return 0;
}

u32 dpp2607_i2c_read(u8 addr)
{
    u8 buffer[4] = {0};
    u32 ret_code = 0;
    u32 data;

    static struct mt_i2c_t i2c;
    i2c.id = DPP2607_I2C_CH;
    i2c.addr = DPP2607_I2C_ADDR;	/* i2c API will shift-left this value to 0xc0 */
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    dpp2607_i2c_write_8bit(0x15, addr);

    ret_code = i2c_read(&i2c, buffer, 4);
    if (ret_code != 0)
    {
        return ret_code;
    }

    data = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
    return data;
}

int dpp2607_rgb_current_is_readed = 0;
int DPP2607_LED_R_CURRENT_DEFAULT = 0x364;
int DPP2607_LED_G_CURRENT_DEFAULT = 0x3FA;
int DPP2607_LED_B_CURRENT_DEFAULT = 0x332;

void dpp2607_i2c_set_brightness(int pwm_level)
{
    int percent = 0;
    u32 data = 0;
    int MIN_PERCENT = 10;
    int MAX_PERCENT = 92*75/100;

    percent = (pwm_level - 40) * 100 / (1023 - 40);
    percent = (MAX_PERCENT - MIN_PERCENT) * percent / 100 + MIN_PERCENT;

    if (percent < MIN_PERCENT)
    {
        percent = MIN_PERCENT;
    }

    if (dpp2607_rgb_current_is_readed == 0)
    {
        data = dpp2607_i2c_read(0x12);
        printf("---- [yan] 2607 brightness [0x12]: 0x%02X ----\n", data);
        if (data != 0)
        {
            DPP2607_LED_R_CURRENT_DEFAULT = data;
        }
        data = 0;
        
        data = dpp2607_i2c_read(0x13);
        printf("---- [yan] 2607 brightness [0x13]: 0x%02X ----\n", data);
        if (data != 0)
        {
            DPP2607_LED_G_CURRENT_DEFAULT = data;
        }
        data = 0;
        
        data = dpp2607_i2c_read(0x14);
        printf("---- [yan] 2607 brightness [0x14]: 0x%02X ----\n", data);
        if (data != 0)
        {
            DPP2607_LED_B_CURRENT_DEFAULT = data;
        }
        
        dpp2607_rgb_current_is_readed = 1;
    }

    dpp2607_i2c_write(0x12, DPP2607_LED_R_CURRENT_DEFAULT * percent / 100); //R
    dpp2607_i2c_write(0x13, DPP2607_LED_G_CURRENT_DEFAULT * percent / 100); //G
    dpp2607_i2c_write(0x14, DPP2607_LED_B_CURRENT_DEFAULT * percent / 100); //B

    dpp2607_i2c_write(0x3A, 0x01);
    dpp2607_i2c_write(0x39, 0x00);
    dpp2607_i2c_write(0x38, 0xD3);
}

//yan screen transform
void dpp2607_i2c_init_screen_transform(void)
{
    char* flip_value = get_env("flip_horizontal");
    int is_flip = 0;
    char* rotate_value = get_env("rotate_180");
    int is_rotate = 0;

    if (flip_value != NULL && flip_value[0] == '1')
    {
        is_flip = 1;
    }

    if (rotate_value != NULL && rotate_value[0] == '1')
    {
        is_rotate = 1;
    }

    if (is_flip == 1 && is_rotate == 1) // rotate 180 & flip horizontal
    {
        #ifdef DPP2607_UP_SIDE_DOWN_SUPPORT
        dpp2607_i2c_write(0x0F, 0x01);
        dpp2607_i2:/c_write(0x10, 0x00);
        #else
        dpp2607_i2c_write(0x0F, 0x00);
        dpp2607_i2c_write(0x10, 0x01);
        #endif
    }
    else if (is_rotate == 1) // rotate 180
    {
        #ifdef DPP2607_UP_SIDE_DOWN_SUPPORT
        dpp2607_i2c_write(0x0F, 0x00);
        dpp2607_i2c_write(0x10, 0x00);
        #else
        dpp2607_i2c_write(0x0F, 0x01);
        dpp2607_i2c_write(0x10, 0x01);
        #endif
    }
    else if (is_flip == 1) // flip horizontal
    {
        #ifdef DPP2607_UP_SIDE_DOWN_SUPPORT
        dpp2607_i2c_write(0x0F, 0x00);
        dpp2607_i2c_write(0x10, 0x01);
        #else
        dpp2607_i2c_write(0x0F, 0x01);
        #endif
    }
    else // normal mode
    {
        #ifdef DPP2607_UP_SIDE_DOWN_SUPPORT
        dpp2607_i2c_write(0x0F, 0x01);
        dpp2607_i2c_write(0x10, 0x01);
        #endif
    }
}
//yan end
#endif
//add end

//leung add 
#define GPIO_FAN_EN          145
//add end

//yan
#define GPIO_USB11_EN           (GPIO50 | 0x80000000)
//yan end


#define GPIO_LED_PWM_GPIO59         (GPIO59 | 0x80000000)
#define GPIO_LED_PWM_GPIO59_M_GPIO   GPIO_MODE_00

//zx add start
void mt65xx_poweroff_charge_backlight_off(void)
{
#if 0
	mt_set_gpio_mode(GPIO_LED_PWM_GPIO59, GPIO_LED_PWM_GPIO59_M_GPIO);
	mt_set_gpio_dir(GPIO_LED_PWM_GPIO59, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LED_PWM_GPIO59, GPIO_OUT_ZERO);
#endif
	enum led_brightness backlight_level = get_cust_led_default_level();
    //mt65xx_leds_brightness_set(MT65XX_LED_TYPE_LCD, backlight_level);
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_LCD, 0);
}
//zx add end




static int mt65xx_led_set_cust(struct cust_mt65xx_led *cust, int level)
{
	unsigned int BacklightLevelSupport = Cust_GetBacklightLevelSupport_byPWM();
	
	if (level > LED_FULL)
		level = LED_FULL;
	else if (level < 0)
		level = 0;

  #ifdef IWLED_SUPPORT
    static bool iwled_init_flag = true;
    if(iwled_init_flag) {
	  mt_vmled_init();
	  iwled_init_flag = false;
    }
  #endif
  
	switch (cust->mode) {
		
		case MT65XX_LED_MODE_PWM:
			if(level == 0)
			{
				//LEDS_INFO("[LEDS]LK: mt65xx_leds_set_cust: enter mt_pwm_disable()\n");
				mt_pwm_disable(cust->data, cust->config_data.pmic_pad);
				return 1;
			}
			if(strcmp(cust->name,"lcd-backlight") == 0)
			{
				if (BacklightLevelSupport == BACKLIGHT_LEVEL_PWM_256_SUPPORT)
					level = brightness_mapping(level);
				else
					level = brightness_mapto64(level);			
			    return brightness_set_pwm(cust->data, level,&cust->config_data);
			}
			else
			{
				return led_set_pwm(cust->data, level);
			}
		
		case MT65XX_LED_MODE_GPIO:
			//brightness_set_pmic(MT65XX_LED_PMIC_NLED_ISINK2, 1);
			return ((cust_brightness_set)(cust->data))(level);
		case MT65XX_LED_MODE_PMIC:
			
			return brightness_set_pmic(cust->data, level);
		case MT65XX_LED_MODE_CUST_LCM:
			return ((cust_brightness_set)(cust->data))(level);
		case MT65XX_LED_MODE_CUST_BLS_PWM:
            //leung add
            if(g_boot_mode == LOW_POWER_OFF_CHARGING_BOOT || g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT || level == 0)
            {           	
                mt_set_gpio_mode(GPIO_FAN_EN, GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_FAN_EN, GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_FAN_EN, GPIO_OUT_ZERO);
                #ifdef DPP3430_SUPPORT
                /*
                mt_set_gpio_mode(GPIO_PAD2005_PROJON,GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_PAD2005_PROJON,GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_PAD2005_PROJON, GPIO_OUT_ZERO);

                mt_set_gpio_mode(GPIO_3430_PARKZ, GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_3430_PARKZ, GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_3430_PARKZ, GPIO_OUT_ZERO);
                */
                #endif

                #if defined(DPP2607_SUPPORT)
                mt_set_gpio_mode(GPIO_PAD1000_RST,GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_PAD1000_RST, GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_PAD1000_RST, GPIO_OUT_ZERO);

                mdelay(200);

                mt_set_gpio_mode(GPIO_PAD1000_PROJON,GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_PAD1000_PROJON,GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_PAD1000_PROJON, GPIO_OUT_ZERO);

                mt_set_gpio_mode(GPIO_2607_LED_EN, GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_2607_LED_EN, GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_2607_LED_EN, GPIO_OUT_ZERO);

                mt_set_gpio_mode(GPIO_2607_PARKZ, GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_2607_PARKZ, GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_2607_PARKZ, GPIO_OUT_ZERO);
                #endif
            }
            else
            {
                mt_set_gpio_mode(GPIO_FAN_EN, GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_FAN_EN, GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_FAN_EN, GPIO_OUT_ONE);
                
                //qxw add
                //lcm_init_transform();
				//current_brightness = 0xFFFFFFFF >> ((level/LED_FULL)* 32 / 100);
				//gpio_set_pwm(PWM2,current_brightness);
                //qxw end
                #ifdef DPP3430_SUPPORT
                /*
                mt_set_gpio_mode(GPIO_PAD2005_PROJON,GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_PAD2005_PROJON,GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_PAD2005_PROJON, GPIO_OUT_ONE);

                mt_set_gpio_mode(GPIO_3430_PARKZ, GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_3430_PARKZ, GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_3430_PARKZ, GPIO_OUT_ONE);
                */
                mdelay(10);

                //dpp3430_i2c_init_duty_ratio();

                //yan screen transform
               // dpp3430_i2c_init_screen_transform();
                //yan end
                //yan keystone correction
                dpp3430_i2c_init_keystone_correction();
                //yan end
				dpp3430_i2c_write_8bit(0x52, 0x07);
				mdelay(10);
				
				dpp3430_i2c_write_8bit(0x07, 0x43);
				mdelay(10);
				
                dpp3430_i2c_write_8bit(0x05, 0x00);
				
                
               // dpp3430_i2c_set_brightness(1023);

                mdelay(10);
                #endif

                #if defined(DPP2607_SUPPORT)
                mt_set_gpio_mode(GPIO_PAD1000_RST,GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_PAD1000_RST, GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_PAD1000_RST, GPIO_OUT_ONE);

                mt_set_gpio_mode(GPIO_2607_INTF, GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_2607_INTF, GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_2607_INTF, GPIO_OUT_ONE);

               // mdelay(50);

                //mt_set_gpio_mode(GPIO_2607_INTF, GPIO_MODE_00);
               // mt_set_gpio_dir(GPIO_2607_INTF, GPIO_DIR_OUT);
             //   mt_set_gpio_out(GPIO_2607_INTF, GPIO_OUT_ZERO);

               // mdelay(150);

                mt_set_gpio_mode(GPIO_PAD1000_PROJON,GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_PAD1000_PROJON,GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_PAD1000_PROJON, GPIO_OUT_ONE);

                mt_set_gpio_mode(GPIO_2607_PARKZ, GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_2607_PARKZ, GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_2607_PARKZ, GPIO_OUT_ONE);
                
                mdelay(200);
                
                //yan screen transform
                dpp2607_i2c_init_screen_transform();
                //yan end
                dpp2607_i2c_set_brightness(1023);
                
                mdelay(10);

                mt_set_gpio_mode(GPIO_2607_LED_EN, GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_2607_LED_EN, GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_2607_LED_EN, GPIO_OUT_ONE);
                #endif
                
                //yan bootup status for IR
                //mt_set_gpio_mode(GPIO_BOOUP_STATUS_FOR_IR, GPIO_MODE_00);
               // mt_set_gpio_dir(GPIO_BOOUP_STATUS_FOR_IR, GPIO_DIR_OUT);
                //mt_set_gpio_out(GPIO_BOOUP_STATUS_FOR_IR, GPIO_OUT_ONE);
                //yan end
                
                //yan
                mt_set_gpio_mode(GPIO_USB11_EN, GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_USB11_EN, GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_USB11_EN, GPIO_OUT_ONE);
                //yan end
            }
            //add end
			return 0;
		case MT65XX_LED_MODE_NONE:
		default:
			break;
	}
	return -1;
}

/****************************************************************************
 * external functions
 ***************************************************************************/
int mt65xx_leds_brightness_set(enum mt65xx_led_type type, enum led_brightness level)
{
	struct cust_mt65xx_led *cust_led_list = get_cust_led_list();

	if (type >= MT65XX_LED_TYPE_TOTAL)
		return -1;

	if (level > LED_FULL)
		level = LED_FULL;
//	else if (level < 0)  //level cannot < 0
//		level = 0;

	if (g_lastlevel[type] != (int)level) {
		g_lastlevel[type] = level;
		dprintf(CRITICAL,"[LEDS]LK: %s level is %d \n\r", cust_led_list[type].name, level);
		return mt65xx_led_set_cust(&cust_led_list[type], level);
	}
	else {
		return -1;
	}

}

void leds_battery_full_charging(void)
{
	//mt65xx_leds_brightness_set(MT65XX_LED_TYPE_GREEN, LED_FULL);
	
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_RED, LED_OFF);
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_GREEN, LED_OFF);
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_BLUE, LED_OFF);
}

void leds_battery_low_charging(void)
{
	//mt65xx_leds_brightness_set(MT65XX_LED_TYPE_RED, LED_FULL);
	
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_RED, LED_OFF);
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_GREEN, LED_OFF);
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_BLUE, LED_OFF);
	
}

void leds_battery_medium_charging(void)
{
	//mt65xx_leds_brightness_set(MT65XX_LED_TYPE_RED, LED_FULL);
	
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_RED, LED_OFF);
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_GREEN, LED_OFF);
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_BLUE, LED_OFF);
}

//qxw add for dc detect
void leds_charging_on_func(void)
{
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_RED, LED_FULL);
}

void leds_charging_off_func(void)
{
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_RED, LED_OFF);
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_GREEN, LED_OFF);
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_BLUE, LED_OFF);
}

int dc_plug_detect_func(void)
{
	int ret = 0;
	mt_set_gpio_mode(GPIO_DC_DET, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_DC_DET, GPIO_DIR_IN);
	ret = mt_get_gpio_in(GPIO_DC_DET);
	
	return ret;	
}
//qxw add end


void leds_init(void)
{ 
		 mt_set_gpio_mode(GPIO_49, GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_49, GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_49, GPIO_OUT_ONE);
                //  mt_set_gpio_mode(GPIO_29, GPIO_MODE_00);
              //  mt_set_gpio_dir(GPIO_29, GPIO_DIR_OUT);
               // mt_set_gpio_out(GPIO_29, GPIO_OUT_ONE);
               // mt_set_gpio_mode(GPIO_29, GPIO_MODE_00);
               // mt_set_gpio_dir(GPIO_29, GPIO_DIR_OUT);
               // mt_set_gpio_out(GPIO_29, GPIO_OUT_ZERO);
               // mdelay(100);
                 mt_set_gpio_mode(GPIO_29, GPIO_MODE_00);
                mt_set_gpio_dir(GPIO_29, GPIO_DIR_OUT);
                mt_set_gpio_out(GPIO_29, GPIO_OUT_ONE);

	LEDS_INFO("[LEDS]LK:GPIO29 leds_init: mt65xx_backlight_off \n\r");
	mt65xx_backlight_off();
}

extern bool g_power_off_charging_led_on;

void leds_deinit(void)
{
	
    if (g_power_off_charging_led_on)
    {
        return;
    }
    LEDS_INFO("[LEDS]LK: leds_deinit: LEDS off \n\r");
	/*
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_RED, LED_OFF);
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_GREEN, LED_OFF);
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_BLUE, LED_OFF);
	*/
}

void mt65xx_backlight_on(void)
{
	enum led_brightness backlight_level = get_cust_led_default_level();
    //qxw add
   
	LEDS_INFO("[LEDS]LK: mt65xx_backlight_on:level =  %d\n\r",backlight_level);
	#if 1
    mt65xx_leds_brightness_set(MT65XX_LED_TYPE_LCD, backlight_level);
	#else
	//zx add start
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_LCD, 80); //??logo ????????80
	//zx add end
	#endif
	// mdelay(50);

    printk("===cxq===mt_leds===%d===%d",mt_get_gpio_out(GPIO_POWER_EN),mt_get_gpio_out(GPIO_PROJ_EN));

     //add by cxq start
    lcm_en();
    //add by cxq end
    //qxw end
}

void mt65xx_backlight_off(void)
{
	LEDS_INFO("[LEDS]LK: mt65xx_backlight_off \n\r");
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_LCD, LED_OFF);
}

