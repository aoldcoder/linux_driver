/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2015. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*/

#include <debug.h>
#include <stdlib.h>
#include <string.h>
#include <video.h>
#include <dev/uart.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <arch/ops.h>
#include <target/board.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_disp_drv.h>
#include <platform/disp_drv.h>
#include <platform/boot_mode.h>
#include <platform/mt_logo.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#include <platform/env.h>
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#include <platform/mt_pmic_wrap_init.h>
#include <platform/mt_i2c.h>
#include <platform/mtk_key.h>
#include <platform/mt_rtc.h>
#include <platform/mt_leds.h>
#include <platform/upmu_common.h>
#include <platform/mtk_wdt.h>
#include <platform/disp_drv_platform.h>

#ifdef MTK_AB_OTA_UPDATER
#include "bootctrl.h"
#endif

#if defined(MTK_SECURITY_SW_SUPPORT)
#include <platform/verified_boot.h>
#include "oemkey.h"
#endif

#if defined(MTK_SECURITY_SW_SUPPORT)
extern u8 g_oemkey[OEM_PUBK_SZ];
#endif

#ifdef LK_DL_CHECK
/*block if check dl fail*/
#undef LK_DL_CHECK_BLOCK_LEVEL
#endif
#define MT_SRAM_REPAIR_SUPPORT
//qxw add
#include <platform/mt_gpio.h>
#include <platform/upmu_common_mt6323.h>
#define GPIO_LED_EN           (GPIO126 | 0x80000000)


#define LCM_EN_0		(GPIO32 | 0x80000000)
#define LCM_EN_1		(GPIO59 | 0x80000000)

#define GPIO_POWER_EN	(GPIO30	| 0x80000000)
#define GPIO_PROJ_EN	(GPIO32	| 0x80000000)
#define GPIO_DCDC_EN	(GPIO86	| 0x80000000)

//#define CHARGEBUMP_EN		(GPIO89 | 0x80000000)

extern void upmu_set_rg_vibr_en(kal_uint32 val);
//qxw end
extern void platform_early_init_timer();
extern void jump_da(u32 addr, u32 arg1, u32 arg2);
extern int i2c_hw_init(void);
extern int mboot_common_load_logo(unsigned long logo_addr, char* filename);
extern int sec_func_init(u64 pl_start_addr);
extern int sec_usbdl_enabled (void);
extern int sec_usbdl_verify_da(unsigned char*, unsigned int, unsigned char*, unsigned int);
extern void mtk_wdt_disable(void);
extern void platform_deinit_interrupts(void);
#ifdef MTK_EMMC_SUPPORT
extern int mmc_get_dl_info(void);
#endif
extern int mmc_legacy_init(u32);
#ifdef MT_SRAM_REPAIR_SUPPORT
extern int repair_sram(void);
#endif
extern bool g_boot_menu;

#ifdef MTK_BATLOWV_NO_PANEL_ON_EARLY
extern kal_bool is_low_battery(kal_int32 val);
extern int hw_charging_get_charger_type(void);
#endif

void platform_uninit(void);
void config_shared_SRAM_size(void);
extern int dev_info_nr_cpu(void);

BOOT_ARGUMENT *g_boot_arg;
BOOT_ARGUMENT boot_addr;
int g_nr_bank;
BI_DRAM bi_dram[MAX_NR_BANK];
unsigned int g_fb_base;
unsigned int g_fb_size;

static int g_dram_init_ret;

//qxw add
extern int dc_plug_detect_func(void);
extern void leds_charging_on_func(void);
extern void leds_charging_off_func(void);
//add end

#if 0//#ifndef MTK_EMMC_SUPPORT
extern flashdev_info gen_FlashTable_p[MAX_FLASH];
unsigned int flash_number;
#endif

int dram_init(void)
{
	int i;
	struct boot_tag *tags;

	/* Get parameters from pre-loader. Get as early as possible
	 * The address of BOOT_ARGUMENT_LOCATION will be used by Linux later
	 * So copy the parameters from BOOT_ARGUMENT_LOCATION to LK's memory region
	 */
	g_boot_arg = &boot_addr;

	if (*(unsigned int *)BOOT_ARGUMENT_LOCATION == BOOT_ARGUMENT_MAGIC) {
		memcpy(g_boot_arg, (void*)BOOT_ARGUMENT_LOCATION, sizeof(BOOT_ARGUMENT));

#if 0//#ifndef MTK_EMMC_SUPPORT
		flash_number = g_boot_arg->flash_number;
		//printf("[xiaolei] [lk] flash number %d\n", flash_number);
		if (0 != flash_number) {
			//gen_FlashTable_p = (flashdev_info*)malloc(flash_number*sizeof(flashdev_info));
			memcpy(gen_FlashTable_p, g_boot_arg->gen_FlashTable, flash_number*sizeof(flashdev_info));
			//printf("[xiaolei] [lk] gen_FlashTable_p ID 0x%x\n", gen_FlashTable_p[0].id[0]);
		}
#endif
	} else {
		g_boot_arg->maggic_number = BOOT_ARGUMENT_MAGIC;
		for (tags = (void *)BOOT_ARGUMENT_LOCATION; tags->hdr.size; tags = boot_tag_next(tags)) {
			switch (tags->hdr.tag) {
				case BOOT_TAG_BOOT_REASON:
					g_boot_arg->boot_reason = tags->u.boot_reason.boot_reason;
					break;
				case BOOT_TAG_BOOT_MODE:
					g_boot_arg->boot_mode = tags->u.boot_mode.boot_mode;
					break;
				case BOOT_TAG_META_COM:
					g_boot_arg->meta_com_type = tags->u.meta_com.meta_com_type;
					g_boot_arg->meta_com_id = tags->u.meta_com.meta_com_id;
					break;
				case BOOT_TAG_LOG_COM:
					g_boot_arg->log_port = tags->u.log_com.log_port;
					g_boot_arg->log_baudrate = tags->u.log_com.log_baudrate;
					g_boot_arg->log_enable = tags->u.log_com.log_enable;
					break;
				case BOOT_TAG_MEM:
					g_boot_arg->dram_rank_num = tags->u.mem.dram_rank_num;
					for (i = 0; i < tags->u.mem.dram_rank_num; i++) {
						g_boot_arg->dram_rank_size[i] = tags->u.mem.dram_rank_size[i];
					}
					break;
				case BOOT_TAG_MD_INFO:
					for (i = 0; i < 4; i++) {
						g_boot_arg->md_type[i] = tags->u.md_info.md_type[i];
					}
					break;
				case BOOT_TAG_BOOT_TIME:
					g_boot_arg->boot_time = tags->u.boot_time.boot_time;
					break;
				case BOOT_TAG_DA_INFO:
					memcpy(&g_boot_arg->da_info, &tags->u.da_info.da_info, sizeof(da_info_t));
					break;
				case BOOT_TAG_SEC_INFO:
					memcpy(&g_boot_arg->sec_limit, &tags->u.sec_info.sec_limit, sizeof(SEC_LIMIT));
					break;
				case BOOT_TAG_PART_NUM:
					g_boot_arg->part_num = tags->u.part_num.part_num;
					break;
				case BOOT_TAG_PART_INFO:
					g_boot_arg->part_info = tags->u.part_info.part_info;  /* only copy the pointer but the contains*/
					break;
				case BOOT_TAG_EFLAG:
					g_boot_arg->e_flag = tags->u.eflag.e_flag;
					break;
				case BOOT_TAG_DDR_RESERVE:
					g_boot_arg->ddr_reserve_enable = tags->u.ddr_reserve.ddr_reserve_enable;
					g_boot_arg->ddr_reserve_success = tags->u.ddr_reserve.ddr_reserve_success;
					g_boot_arg->ddr_reserve_ready = tags->u.ddr_reserve.ddr_reserve_ready;
					break;
				case BOOT_TAG_DRAM_BUF:
					g_boot_arg->dram_buf_size = tags->u.dram_buf.dram_buf_size;
					break;
                    //yan read flash id for factory_test
                case BOOT_TAG_CUST_INFO:
                    //check_tag_size(tags->hdr.size,boot_tag_size(boot_tag_cust_info),tags->hdr.tag);
                    memcpy(g_boot_arg->cust_param, tags->u.cust_info.cust_param, sizeof(char[32]));
                    break;
                    //yan end
				default:
					break;
			}
		}
	}


#ifdef MACH_FPGA
	g_nr_bank = 2;
	bi_dram[0].start = DRAM_PHY_ADDR + RIL_SIZE;
	bi_dram[0].size = (256 * 1024 * 1024) - RIL_SIZE;
	bi_dram[1].start = bi_dram[0].start + bi_dram[0].size;
	bi_dram[1].size = (256 * 1024 * 1024);
#else
	g_nr_bank = g_boot_arg->dram_rank_num;

	if (g_nr_bank == 0 || g_nr_bank > MAX_NR_BANK) {
		g_dram_init_ret = -1;
		//dprintf(CRITICAL, "[LK ERROR] DRAM bank number is not correct!!!");
		//while (1) ;
		return -1;
	}

#ifndef CUSTOM_CONFIG_MAX_DRAM_SIZE
	/* return the actual DRAM info */
	bi_dram[0].start = DRAM_PHY_ADDR + RIL_SIZE;
	bi_dram[0].size = g_boot_arg->dram_rank_size[0] - RIL_SIZE;
	for (i = 1; i < g_nr_bank; i++) {
		bi_dram[i].start = bi_dram[i-1].start + bi_dram[i-1].size;
		bi_dram[i].size = g_boot_arg->dram_rank_size[i];
	}
//#elif (CUSTOM_CONFIG_MAX_DRAM_SIZE < 0x10000000)
//#error "DRAM size < 0x10000000" /* DRAM is less than 256MB, trigger build error */
#else
#endif

#endif

	return 0;
}

/*******************************************************
 * Routine: memory_size
 * Description: return DRAM size to LCM driver
 ******************************************************/
u64 physical_memory_size(void)
{
	int i;
	unsigned long long size = 0;

	for (i = 0; i < g_boot_arg->orig_dram_info.rank_num; i++) {
		size += g_boot_arg->orig_dram_info.rank_info[i].size;
	}

	return size;
}

u32 memory_size(void)
{
	unsigned long long size = physical_memory_size();

	while (((unsigned long long)DRAM_PHY_ADDR + size) > 0x100000000ULL) {
		size -= (unsigned long long)(1024*1024*1024);
	}

	return (unsigned int)size;
}

void sw_env()
{
#ifdef LK_DL_CHECK
#ifdef MTK_EMMC_SUPPORT
	int dl_status = 0;
	dl_status = mmc_get_dl_info();
	dprintf(INFO, "mt65xx_sw_env--dl_status: %d\n", dl_status);
	if (dl_status != 0) {
		video_printf("=> TOOL DL image Fail!\n");
		dprintf(CRITICAL, "TOOL DL image Fail\n");
#ifdef LK_DL_CHECK_BLOCK_LEVEL
		dprintf(CRITICAL, "uboot is blocking by dl info\n");
		while (1) ;
#endif
	}
#endif
#endif

#ifndef USER_BUILD
	switch (g_boot_mode) {
		case META_BOOT:
			video_printf(" => META MODE\n");
			break;
		case FACTORY_BOOT:
			video_printf(" => FACTORY MODE\n");
			break;
		case RECOVERY_BOOT:
			video_printf(" => RECOVERY MODE\n");
			break;
		case SW_REBOOT:
			//video_printf(" => SW RESET\n");
			break;
		case NORMAL_BOOT:
			//if(g_boot_arg->boot_reason != BR_RTC && get_env("hibboot") != NULL && atoi(get_env("hibboot")) == 1)
			if (get_env("hibboot") != NULL && atoi(get_env("hibboot")) == 1)
				video_printf(" => HIBERNATION BOOT\n");
			else
				video_printf(" => NORMAL BOOT\n");
			break;
		case ADVMETA_BOOT:
			video_printf(" => ADVANCED META MODE\n");
			break;
		case ATE_FACTORY_BOOT:
			video_printf(" => ATE FACTORY MODE\n");
			break;
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
		case KERNEL_POWER_OFF_CHARGING_BOOT:
			video_printf(" => POWER OFF CHARGING MODE\n");
			break;
		case LOW_POWER_OFF_CHARGING_BOOT:
			video_printf(" => LOW POWER OFF CHARGING MODE\n");
			break;
#endif
		case ALARM_BOOT:
			video_printf(" => ALARM BOOT\n");
			break;
		case FASTBOOT:
			video_printf(" => FASTBOOT mode...\n");
			break;
		default:
			video_printf(" => UNKNOWN BOOT\n");
	}
	return;
#endif

#ifdef USER_BUILD
	if (g_boot_mode == FASTBOOT)
		video_printf(" => FASTBOOT mode...\n");
	return;
#endif
}

void platform_init_mmu_mappings(void)
{
	unsigned int addr;
	unsigned int dram_size = 0;

	dram_init();
	arm_mmu_init();
	dram_size = physical_memory_size();
	
	/* map dram region */
	for (addr = 0; addr < dram_size; addr += (1024*1024)) {
		/*virtual to physical 1-1 mapping*/
		arm_mmu_map_section(bi_dram[0].start+addr,bi_dram[0].start+addr, MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE | MMU_MEMORY_AP_READ_WRITE);
	}
	arch_enable_mmu();	//enable mmu after setup page table to avoid cpu prefetch which may bring on emi violation 
}

void platform_init_mmu(void)
{
	unsigned long long addr;
	unsigned int vaddr;
	unsigned long long dram_size;

	/* configure available RAM banks */
	dram_init();

	dram_size = physical_memory_size();

	if (((unsigned long long)DRAM_PHY_ADDR + dram_size) <= 0x100000000ULL) {
		arm_mmu_init();

		for (addr = 0; addr < dram_size; addr += (1024*1024)) {
			/*virtual to physical 1-1 mapping*/
			arm_mmu_map_section(bi_dram[0].start+addr, bi_dram[0].start+addr, MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE | MMU_MEMORY_AP_READ_WRITE);
		}
	} else {
		arm_mmu_lpae_init();

		for (addr = 0; addr < dram_size; addr += (unsigned long long)(1024*1024*1024)) {
			vaddr = (bi_dram[0].start + addr < 0x100000000ULL) ? (unsigned int)(bi_dram[0].start + addr) : (0xC0000000);
			arm_mmu_map_block(bi_dram[0].start+addr, vaddr, LPAE_MMU_MEMORY_TYPE_NORMAL_WRITE_BACK);
		}


	}
	arch_enable_mmu();  //enable mmu after setup page table to avoid cpu prefetch which may bring on emi violation
}


void platform_k64_check(void)
{
	dprintf(CRITICAL, "kernel_boot_opt=%d\n", g_boot_arg->kernel_boot_opt);

	switch (g_boot_arg->kernel_boot_opt) {
		case BOOT_OPT_64S3:
		case BOOT_OPT_64S1:
		case BOOT_OPT_64N2:
		case BOOT_OPT_64N1:
			g_is_64bit_kernel = 1;
			dprintf(CRITICAL, "64Bit Kernel\n");
			break;

		case BOOT_OPT_32S3:
		case BOOT_OPT_32S1:
		case BOOT_OPT_32N2:
		case BOOT_OPT_32N1:
		/* maybe need to do something in the feature*/
		default:
			g_is_64bit_kernel = 0;
			dprintf(CRITICAL, "32Bit Kernel\n");
			break;
	}
}


void platform_early_init(void)
{
#ifdef MT_SRAM_REPAIR_SUPPORT
	int repair_ret;
#endif
#ifdef LK_PROFILING
#ifdef MT_SRAM_REPAIR_SUPPORT
	unsigned int time_repair_sram;
#endif
	unsigned int time_wdt_early_init;
	unsigned int time_led_init;
	unsigned int time_pmic_init;
	unsigned int time_platform_early_init;

	time_platform_early_init = get_timer(0);
#endif
	platform_init_interrupts();

	platform_early_init_timer();

#ifndef MACH_FPGA
	mt_gpio_set_default();
#endif

	/* initialize the uart */
	uart_init_early();

	platform_k64_check();

	if (g_dram_init_ret < 0) {
		dprintf(CRITICAL, "[LK ERROR] DRAM bank number is not correct!!!\n");
		while (1) ;
	}

	//i2c_v1_init();

#ifdef LK_PROFILING
	time_wdt_early_init = get_timer(0);
#endif
	mtk_wdt_init();
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- WDT Init  takes %d ms -------- \n", (int)get_timer(time_wdt_early_init));
#endif

#ifdef MT_SRAM_REPAIR_SUPPORT
#ifdef LK_PROFILING
	time_repair_sram = get_timer(0);
#endif
	repair_ret = repair_sram();
	if (repair_ret != 0) {
		dprintf(CRITICAL, "Sram repair failed %d\n", repair_ret);
		while (1);
	}
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- Repair SRAM takes %d ms -------- \n", (int)get_timer(time_repair_sram));
#endif
#endif

	//i2c init
	i2c_hw_init();

#ifdef MACH_FPGA
	mtk_timer_init();  // GPT4 will be initialized at PL after
	mtk_wdt_disable();  // WDT will be triggered when uncompressing linux image on FPGA
#endif



#ifndef MACH_FPGA
#ifdef LK_PROFILING
	time_led_init = get_timer(0);
#endif
	leds_init();
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- led init takes %d ms -------- \n", (int)get_timer(time_led_init));
#endif
#endif

#if 0//#ifndef MTK_EMMC_SUPPORT
	flash_number = g_boot_arg->flash_number;
	printf("-------- 0x%x 0x%x\n",g_boot_arg->gen_FlashTable, gen_FlashTable_p);
	if (0 != flash_number) {
		int k= 0;
		unsigned int* q = (unsigned int*)gen_FlashTable_p;
		for (k = 0 ; k<(flash_number*sizeof(flashdev_info))/4; k++)
			printf("0x%x, ",*q++);
	}
	printf("\n[PROFILE] ------- flash_number init takes %d -------- \n",flash_number);
#endif

	// Workaround by Peng
	//pwrap_init_lk();
	//pwrap_init_for_early_porting();

#ifdef LK_PROFILING
	time_pmic_init = get_timer(0);
#endif
	pmic_init();
	/*
	    // Workaround by Weiqi
	    mt6331_upmu_set_rg_vgp1_en(1);
	    mt6331_upmu_set_rg_vcam_io_en(1);
	*/
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- pmic_init takes %d ms -------- \n", (int)get_timer(time_pmic_init));
#endif

#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- platform_early_init takes %d ms -------- \n", (int)get_timer(time_platform_early_init));
#endif
}

extern void mt65xx_bat_init(void);
#if defined (MTK_KERNEL_POWER_OFF_CHARGING)

int kernel_charging_boot(void)
{
	if ((g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT || g_boot_mode == LOW_POWER_OFF_CHARGING_BOOT) && upmu_is_chr_det() == KAL_TRUE) {
		dprintf(INFO,"[%s] Kernel Power Off Charging with Charger/Usb \n", __func__);
		return  1;
	} else if ((g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT || g_boot_mode == LOW_POWER_OFF_CHARGING_BOOT) && upmu_is_chr_det() == KAL_FALSE) {
		dprintf(INFO,"[%s] Kernel Power Off Charging without Charger/Usb \n", __func__);
		return -1;
	} else
		return 0;
}
#endif

//qxw add
void tp_isink1_func(int mode){
				
		upmu_set_rg_drv_32k_ck_pdn(0x0);
        upmu_set_rg_isink1_ck_pdn(0);
        upmu_set_rg_isink1_ck_sel(0);
        upmu_set_isink_ch1_mode(7);
        upmu_set_isink_ch1_step(0x0);  /* 0x3 16mA */
        upmu_set_isink_dim1_duty(0);  /* 15 */
        upmu_set_isink_dim1_fsel(0);
        upmu_set_isink_ch1_en(mode);                      
}

void power_leds_init(void){
				
				upmu_set_rg_drv_32k_ck_pdn(0x0);
        upmu_set_rg_isink2_ck_pdn(0);
        upmu_set_rg_isink2_ck_sel(0);
        upmu_set_isink_ch2_mode(ISINK_PWM_MODE);
        upmu_set_isink_ch2_step(0x0);
        upmu_set_isink_dim2_duty(0);
        upmu_set_isink_dim2_fsel(0);
        upmu_set_isink_ch2_en(0x01); 
        
        mt_set_gpio_mode(GPIO_LED_EN, GPIO_MODE_00);
        mt_set_gpio_dir(GPIO_LED_EN, GPIO_DIR_OUT);
        mt_set_gpio_out(GPIO_LED_EN, GPIO_OUT_ONE);                   
}

/*void chargeBumpEnable(void)
{
		mt_set_gpio_mode(CHARGEBUMP_EN, GPIO_MODE_00);
		mt_set_gpio_dir(CHARGEBUMP_EN, GPIO_DIR_OUT);
		mt_set_gpio_out(CHARGEBUMP_EN, GPIO_OUT_ONE);
}

void chargeBumpDisable(void)
{
		mt_set_gpio_mode(CHARGEBUMP_EN, GPIO_MODE_00);
		mt_set_gpio_dir(CHARGEBUMP_EN, GPIO_DIR_OUT);
		mt_set_gpio_out(CHARGEBUMP_EN, GPIO_OUT_ONE);
}*/

void lcm_en(void)
{
		mt_set_gpio_mode(LCM_EN_0, GPIO_MODE_00);
		mt_set_gpio_dir(LCM_EN_0, GPIO_DIR_OUT);
		mt_set_gpio_out(LCM_EN_0, GPIO_OUT_ONE);  
		
		//mt_set_gpio_mode(LCM_EN_1, GPIO_MODE_00);
		//mt_set_gpio_dir(LCM_EN_1, GPIO_DIR_OUT);
		//mt_set_gpio_out(LCM_EN_1, GPIO_OUT_ONE);
}
//qxw end


void gpio_set_status()
{
	mt_set_gpio_mode(GPIO_POWER_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_POWER_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_POWER_EN, GPIO_OUT_ONE);
	mdelay(20);
	mt_set_gpio_mode(LCM_EN_0, GPIO_MODE_00);
	mt_set_gpio_dir(LCM_EN_0, GPIO_DIR_OUT);
	mt_set_gpio_out(LCM_EN_0, GPIO_OUT_ONE);
	mdelay(300);
	mt_set_gpio_mode(GPIO_DCDC_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_DCDC_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_DCDC_EN, GPIO_OUT_ONE);
}

void platform_init(void)
{
	//  printk("===cxq===platform_init===%d===%d====",mt_get_gpio_out(GPIO_POWER_EN),mt_get_gpio_out(GPIO_PROJ_EN));

	//  gpio_set_status();

#ifdef LK_PROFILING
	unsigned int time_nand_emmc;
	unsigned int time_env;
	unsigned int time_disp_init;
	unsigned int time_load_logo;
	unsigned int time_backlight;
	unsigned int time_boot_mode;
#ifdef MTK_SECURITY_SW_SUPPORT
	unsigned int time_security_init;
#endif
	unsigned int time_bat_init;
	unsigned int time_RTC_boot_Check;
	unsigned int time_show_logo;
	unsigned int time_sw_env;
	unsigned int time_platform_init;
#ifdef VDEC_LDVT_RESERVED_MEMORY_SIZE
	unsigned int g_vdec_base;
#endif

	time_platform_init = get_timer(0);
#endif
	u64 pl_start_addr = 0;
	plinfo_get_brom_header_block_size(&pl_start_addr);

	dprintf(CRITICAL, "platform_init()\n");

	// printk("===cxq===platform_init===333==%d===%d",mt_get_gpio_out(GPIO_POWER_EN),mt_get_gpio_out(GPIO_PROJ_EN));

#ifdef DUMMY_AP
	dummy_ap_entry();
#endif

#ifdef LK_PROFILING
	time_nand_emmc = get_timer(0);
#endif
#ifdef MTK_EMMC_SUPPORT
	mmc_legacy_init(1);
#else
//#ifndef MACH_FPGA
#ifdef MTK_MLC_NAND_SUPPORT
	nand_init();
	nand_driver_test();
#endif
#endif
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- NAND/EMMC init takes %d ms -------- \n", (int)get_timer(time_nand_emmc));
#endif

#ifdef MTK_AB_OTA_UPDATER
	/* get A/B system parameter before load dtb from boot image */
	get_AB_OTA_param();
#endif

#ifdef MTK_KERNEL_POWER_OFF_CHARGING
	if ((g_boot_arg->boot_reason == BR_USB) && (upmu_is_chr_det() == KAL_FALSE)) {
		dprintf(INFO, "[%s] Unplugged Charger/Usb between Pre-loader and Uboot in Kernel Charging Mode, Power Off \n", __func__);
		mt6575_power_off();
	}
#endif

#ifdef LK_PROFILING
	time_env = get_timer(0);
#endif
	env_init();
	print_env();

	// printk("===cxq===platform_init==444==%d===%d",mt_get_gpio_out(GPIO_POWER_EN),mt_get_gpio_out(GPIO_PROJ_EN));
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- ENV init takes %d ms -------- \n", (int)get_timer(time_env));
#endif

#ifdef LK_PROFILING
	time_disp_init = get_timer(0);
#endif

	/* initialize the frame buffet information */
	g_fb_size = mt_disp_get_vram_size();
#if 0
	g_fb_base = memory_size() - g_fb_size + DRAM_PHY_ADDR;
#else

	if (g_is_64bit_kernel) {
		g_fb_base = mblock_reserve(&g_boot_arg->mblock_info, g_fb_size, 0x200000, 0x100000000, RANKMAX);
		g_fb_base = ALIGN_TO(g_fb_base,0x200000); // size 2MB align
	} else {
		g_fb_base = mblock_reserve(&g_boot_arg->mblock_info, g_fb_size, 0x100000, 0x100000000, RANKMAX);
	}

	if (!g_fb_base) {
		/* ERROR */
	}
#endif

	dprintf(CRITICAL, "FB base = 0x%x, FB size = %d\n", g_fb_base, g_fb_size);

#ifdef VDEC_LDVT_RESERVED_MEMORY_SIZE
	g_vdec_base = mblock_reserve(&g_boot_arg->mblock_info, VDEC_LDVT_RESERVED_MEMORY_SIZE, 0x200000, 0x100000000, RANKMAX);
	g_vdec_base = ALIGN_TO(g_vdec_base,0x200000);

	dprintf(CRITICAL, "VDEC base = 0x%x, VDEC size = %d\n", g_vdec_base, 0x10000000);
#endif

#if 0
	mt_disp_init((void *)g_fb_base);
	/* show black picture fisrtly in case of  backlight is on before nothing is drawed*/
	mt_disp_fill_rect(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT, 0x0);

	mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
	
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- disp init takes %d ms -------- \n", (int)get_timer(time_disp_init));
#endif

#ifdef LK_PROFILING
	time_load_logo = get_timer(0);
#endif
	drv_video_init();
	
	mboot_common_load_logo((unsigned long)mt_get_logo_db_addr_pa(), "logo");
	
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- load_logo takes %d ms -------- \n", (int)get_timer(time_load_logo));
#endif
#endif	
	/*for kpd pmic mode setting*/
	set_kpd_pmic_mode();

#ifndef MACH_FPGA
#ifdef LK_PROFILING
	time_boot_mode = get_timer(0);
#endif

 printk("===cxq===boot_mode_select===1111===%d===%d",mt_get_gpio_out(GPIO_POWER_EN),mt_get_gpio_out(GPIO_PROJ_EN));
	boot_mode_select();
#if 0
	#ifdef MTK_KERNEL_POWER_OFF_CHARGING
		
		if(kernel_charging_boot() == 0)
			{
				// gpio_set_status();
			}
	#endif
#endif

#if 1
	mt_disp_init((void *)g_fb_base);
	/* show black picture fisrtly in case of  backlight is on before nothing is drawed*/
	mt_disp_fill_rect(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT, 0x0);

	mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
	
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- disp init takes %d ms -------- \n", (int)get_timer(time_disp_init));
#endif

#ifdef LK_PROFILING
	time_load_logo = get_timer(0);
#endif
	drv_video_init();
	
	mboot_common_load_logo((unsigned long)mt_get_logo_db_addr_pa(), "logo");
	
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- load_logo takes %d ms -------- \n", (int)get_timer(time_load_logo));
#endif

#endif

 printk("===cxq===boot_mode_select===2222===%d===%d",mt_get_gpio_out(GPIO_POWER_EN),mt_get_gpio_out(GPIO_PROJ_EN));

// printk("===cxq===boot_mode_select===222===%d===%d",mt_get_gpio_out(GPIO_POWER_EN),mt_get_gpio_out(GPIO_PROJ_EN));
	//qxw add
	printf("***lcm_en start***\n");
	//lcm_en();
	
	printf("***lcm_en end***\n");
	//qxw add end
	
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- boot mode select takes %d ms -------- \n", (int)get_timer(time_boot_mode));
#endif
#endif

#ifdef MTK_SECURITY_SW_SUPPORT
#ifdef LK_PROFILING
	time_security_init = get_timer(0);
#endif

	/* initialize security library */
	sec_func_init(pl_start_addr);

#ifdef LK_PROFILING
	dprintf(INFO,"[PROFILE] ------- Security init takes %d ms -------- \n", (int)get_timer(time_security_init));
#endif

	seclib_set_oemkey(g_oemkey, OEM_PUBK_SZ);
	if (0 != img_auth_stor(SBOOT_PART_LOGO_NAME, NULL)) {
		dprintf(CRITICAL, "<ASSERT> %s:line %d\n", __FILE__, __LINE__);
		while(1);
	}

	if (0 != img_auth_stor(SBOOT_PART_DTBO_NAME, NULL)) {
		dprintf(CRITICAL, "<ASSERT> %s:line %d\n", __FILE__, __LINE__);
		while(1);
	}
#endif

	/*Show download logo & message on screen */
	if (g_boot_arg->boot_mode == DOWNLOAD_BOOT) {
		dprintf(CRITICAL, "[LK] boot mode is DOWNLOAD_BOOT\n");

#ifdef MTK_SECURITY_SW_SUPPORT
		/* verify da before jumping to da*/
		if (sec_usbdl_enabled()) {
			u8  *da_addr = (u8*)g_boot_arg->da_info.addr;
			u32 da_len   = g_boot_arg->da_info.len;
			u32 sig_len  = g_boot_arg->da_info.sig_len;
			u8  *sig_addr = (unsigned char *)da_addr + (da_len - sig_len);

			if (da_len == 0 || sig_len == 0) {
				dprintf(INFO, "[LK] da argument is invalid\n");
				dprintf(INFO, "da_addr = 0x%x\n", (int)da_addr);
				dprintf(INFO, "da_len  = 0x%x\n", da_len);
				dprintf(INFO, "sig_len = 0x%x\n", sig_len);
			}

			if (sec_usbdl_verify_da(da_addr, (da_len - sig_len), sig_addr, sig_len)) {
				/* da verify fail */
				video_printf(" => Not authenticated tool, download stop...\n");
				while (1); /* fix me, should not be infinite loop in lk */
			}
		} else
#endif
		{
			dprintf(INFO, " DA verification disabled...\n");
		}

		mt_disp_show_boot_logo();
		video_printf(" => Downloading...\n");
		dprintf(CRITICAL, "enable backlight after show bootlogo! \n");
		mt65xx_backlight_on(); 	  

		mtk_wdt_disable(); //Disable wdt before jump to DA
		platform_uninit();
#ifdef HAVE_CACHE_PL310
		l2_disable();
#endif
		arch_disable_cache(UCACHE);
		arch_disable_mmu();
#ifdef ENABLE_L2_SHARING
		config_shared_SRAM_size();
#endif

		jump_da(g_boot_arg->da_info.addr, g_boot_arg->da_info.arg1, g_boot_arg->da_info.arg2);
	}

#ifdef LK_PROFILING
	time_bat_init = get_timer(0);
#endif
	mt65xx_bat_init();
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- battery init takes %d ms -------- \n", (int)get_timer(time_bat_init));
#endif

#ifndef CFG_POWER_CHARGING
#ifdef LK_PROFILING
	time_RTC_boot_Check = get_timer(0);
#endif
	/* NOTE: if define CFG_POWER_CHARGING, will rtc_boot_check() in mt65xx_bat_init() */
	rtc_boot_check(false);
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- RTC boot check Init  takes %d ms -------- \n", (int)get_timer(time_RTC_boot_Check));
#endif
#endif

#ifdef LK_PROFILING
	time_show_logo = get_timer(0);
#endif
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
	if (kernel_charging_boot() == 1) {
			//zxw add start
			dprintf(CRITICAL, "[PROFILE] ------- poweroff charge test -------- \n");
			if(g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT)
			{
				mt65xx_poweroff_charge_backlight_off();
			}
			//zxw add end
	
#ifdef MTK_BATLOWV_NO_PANEL_ON_EARLY
		CHARGER_TYPE CHR_Type_num = CHARGER_UNKNOWN;
		CHR_Type_num = hw_charging_get_charger_type();
		if ((g_boot_mode != LOW_POWER_OFF_CHARGING_BOOT) ||
		        ((CHR_Type_num != STANDARD_HOST) && (CHR_Type_num != NONSTANDARD_CHARGER))) {
#endif
			mt_disp_power(TRUE);
			mt_disp_show_low_battery();
			#if 0
			mt65xx_leds_brightness_set(6, 110);
			#else
			//
			mt65xx_poweroff_charge_backlight_off();
			//
			#endif
			
			
#ifdef MTK_BATLOWV_NO_PANEL_ON_EARLY
		}
#endif
	} else if (g_boot_mode != KERNEL_POWER_OFF_CHARGING_BOOT && g_boot_mode != LOW_POWER_OFF_CHARGING_BOOT) {
		if (g_boot_mode != ALARM_BOOT && (g_boot_mode != FASTBOOT)) {
			mt_disp_show_boot_logo();
		}
	}
#else
	if (g_boot_mode != ALARM_BOOT && (g_boot_mode != FASTBOOT)) {
		mt_disp_show_boot_logo();
	}
#endif
#ifdef LK_PROFILING
	time_backlight = get_timer(0);
#endif

#ifdef MTK_BATLOWV_NO_PANEL_ON_EARLY
	if (!is_low_battery(0)) {
#endif
		mt65xx_backlight_on();
		//zx add start
		if (kernel_charging_boot() == 1) {
			dprintf(CRITICAL, "[PROFILE] ------- poweroff charge test222 -------- \n");
			//if(g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT)
			{
				mt65xx_poweroff_charge_backlight_off();
				dprintf(CRITICAL, "[PROFILE] ------- poweroff charge test3333 -------- \n");
			}
		}
		//zx add end
		//pwm need display sof
     
    //qxw add 
    //leds_charging_on_func();
    
    if(dc_plug_detect_func() == 0){
    		dprintf(CRITICAL, "[PROFILE] ------- poweron plugin -------- \n");
    		leds_charging_on_func();
    }else{
    		dprintf(CRITICAL, "[PROFILE] ------- poweron plugout -------- \n");
    		leds_charging_off_func();
    }
    // forcing red led on add for without bat case
    leds_charging_on_func();

    /*    
    		tp_isink1_func(1);
				power_leds_init();
				
				//mdelay(1000);
				//tp_isink1_func(1);
				upmu_set_rg_vibr_en(1);
				
				//mdelay(1000);
				//tp_isink1_func(0);
				upmu_set_rg_vibr_en(0);
		*/
    //qxw end
    
		mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
#ifdef MTK_BATLOWV_NO_PANEL_ON_EARLY
	}
#endif

#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- backlight takes %d ms -------- \n", (int)get_timer(time_backlight));
#endif

#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- show logo takes %d ms -------- \n", (int)get_timer(time_show_logo));
#endif

#ifndef MACH_FPGA
#ifdef LK_PROFILING
	time_sw_env = get_timer(0);
#endif
	sw_env();
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- sw_env takes %d ms -------- \n", (int)get_timer(time_sw_env));
#endif
#endif

#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- platform_init takes %d ms -------- \n", (int)get_timer(time_platform_init));
#endif
}

void platform_uninit(void)
{
#ifndef MACH_FPGA
	leds_deinit();
#endif
	platform_deinit_interrupts();
	return;
}

#ifdef ENABLE_L2_SHARING
#define ADDR_CA7L_CACHE_CONFIG_MP(x) (CA7MCUCFG_BASE + 0x200 * x)
#define L2C_SIZE_CFG_OFFSET  8
#define L2C_SHARE_EN_OFFSET  12
/* 4'b1111: 2048KB(not support)
 * 4'b0111: 1024KB(not support)
 * 4'b0011: 512KB
 * 4'b0001: 256KB
 * 4'b0000: 128KB (not support)
 */

int is_l2_need_config(void)
{
	volatile unsigned int cache_cfg, addr;

	addr = ADDR_CA7L_CACHE_CONFIG_MP(0);
	cache_cfg = DRV_Reg32(addr);
	cache_cfg = cache_cfg >> L2C_SIZE_CFG_OFFSET;

	/* only read 256KB need to be config.*/
	if ((cache_cfg &(0x7)) == 0x1) {
		return 1;
	}
	return 0;
}

void cluster_l2_share_enable(int cluster)
{
	volatile unsigned int cache_cfg, addr;

	addr = ADDR_CA7L_CACHE_CONFIG_MP(cluster);
	/* set L2C size to 256KB */
	cache_cfg = DRV_Reg32(addr);
	cache_cfg &= (~0x7) << L2C_SIZE_CFG_OFFSET;
	cache_cfg |= 0x1 << L2C_SIZE_CFG_OFFSET;

	/* enable L2C_share_en. Sram only for other to use*/
	cache_cfg |= (0x1 << L2C_SHARE_EN_OFFSET);
	DRV_WriteReg32(addr, cache_cfg);
}

void cluster_l2_share_disable(int cluster)
{
	volatile unsigned int cache_cfg, addr;

	addr = ADDR_CA7L_CACHE_CONFIG_MP(cluster);
	/* set L2C size to 512KB */
	cache_cfg = DRV_Reg32(addr);
	cache_cfg &= (~0x7) << L2C_SIZE_CFG_OFFSET;
	cache_cfg |= 0x3 << L2C_SIZE_CFG_OFFSET;
	DRV_WriteReg32(addr, cache_cfg);

	/* disable L2C_share_en. Sram only for cpu to use*/
	cache_cfg &= ~(0x1 << L2C_SHARE_EN_OFFSET);
	DRV_WriteReg32(addr, cache_cfg);
}

/* config L2 cache and sram to its size */
void config_L2_size(void)
{
	int cluster;

	if (is_l2_need_config()) {
		/*
		 * Becuase mcu config is protected.
		 * only can write in secutity mode
		 */

		if (dev_info_nr_cpu() == 6) {
			cluster_l2_share_disable(0);
			cluster_l2_share_enable(1);
		}

		else {
			for (cluster = 0; cluster < 2; cluster++) {
				cluster_l2_share_disable(cluster);
			}
		}
	}
}

/* config SRAM back from L2 cache for DA relocation */
void config_shared_SRAM_size(void)
{
	int cluster;

	if (is_l2_need_config()) {
		/*
		 * Becuase mcu config is protected.
		 * only can write in secutity mode
		 */

		for (cluster = 0; cluster < 2; cluster++) {
			cluster_l2_share_enable(cluster);
		}
	}
}

#ifdef NEW_MEMORY_RESERVED_MODEL

/*
 * reserve a memory from mblock
 * @mblock_info: address of mblock_info
 * @size: size of memory
 * @align: alignment, not implemented
 * @limit: address limit. Must higher than return address + size
 * @rank: preferable rank, the returned address is in rank or lower ranks
 * It returns as high rank and high address as possible. (consider rank first)
 */
u64 mblock_reserve(mblock_info_t *mblock_info, u64 size, u64 align, u64 limit,
                   enum reserve_rank rank)
{
	int i, max_rank, target = -1;
	u64 start, sz, max_addr = 0;

	if (size & (0x200000 - 1)) {
		dprintf(CRITICAL,"warning: size is not 2MB aligned\n");
	}

	if (rank == RANK0) {
		/* reserve memory from rank 0 */
		max_rank = 0;
	} else {
		/* reserve memory from any possible rank */
		/* mblock_num >= nr_ranks is true */
		max_rank = mblock_info->mblock_num - 1;
	}

	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		dprintf(CRITICAL,"mblock[%d].start: 0x%llx, sz: 0x%llx, limit: 0x%llx, "
		        "max_addr: 0x%llx, max_rank: %d, target: %d, "
		        "mblock[].rank: %d\n",
		        i, start, sz, limit, max_addr, max_rank,
		        target, mblock_info->mblock[i].rank);
		dprintf(CRITICAL,"mblock_reserve dbg[%d]: %d, %d, %d, %d\n",
		        i, (start + sz <= limit),
		        (mblock_info->mblock[i].rank <= max_rank),
		        (start + sz > max_addr),
		        (sz >= size));
		if ((start + sz <= limit) &&
		        (mblock_info->mblock[i].rank <= max_rank) &&
		        (start + sz > max_addr) &&
		        (sz >= size)) {
			max_addr = start + sz;
			target = i;
		}
	}

	if (target < 0) {
		dprintf(CRITICAL,"mblock_reserve error\n");
		return 0;
	}

	mblock_info->mblock[target].size -= size;

	dprintf(CRITICAL,"mblock_reserve: %llx - %llx from mblock %d\n",
	        (mblock_info->mblock[target].start
	         + mblock_info->mblock[target].size),
	        (mblock_info->mblock[target].start
	         + mblock_info->mblock[target].size + size),
	        target);


	return mblock_info->mblock[target].start +
	       mblock_info->mblock[target].size;
}
#endif //NEW_MEMORY_RESERVED_MODEL
#endif

int platform_skip_hibernation(void)
{
	if (g_boot_menu == true)
		return 1;

	switch (g_boot_arg->boot_reason) {
#if 0 // let schedule power on to go hiberantion bootup process
		case BR_RTC:
#endif
		case BR_WDT:
		case BR_WDT_BY_PASS_PWK:
		case BR_WDT_SW:
		case BR_WDT_HW:
			return 1;
	}

	return 0;
}
