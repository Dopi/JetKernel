/* linux/drivers/media/video/samsung/s3c_ipc.c
 *
 * ipc Support file for FIMC driver
 *
 * Youngmok Song, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include <asm/io.h>
#include <asm/memory.h>
#include <plat/clock.h>
#include <plat/regs-ipc.h>
#include <plat/fimc-ipc.h>

#include "fimc-ipc.h"
#include "ipc_table.h"

#define OFF	0
#define ON	1

#define IN_SC_MAX_WIDTH		1024
#define IN_SC_MAX_HEIGHT	768

static struct s3c_ipc_info *s3c_ipc;

static ipc_enhancingvariable ipc_enh_var;

ipc_source 		ipc_input_src;
ipc_destination 	ipc_output_dst;
ipc_controlvariable 	ipc_con_var;

static struct s3c_platform_ipc *to_ipc_plat(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	return (struct s3c_platform_ipc *) pdev->dev.platform_data;
}

static int s3c_ipc_set_info(void)
{
	s3c_ipc = (struct s3c_ipc_info *) \
			kmalloc(sizeof(struct s3c_ipc_info), GFP_KERNEL);
	if (!s3c_ipc) {
		err("no memory for configuration\n");
		return -ENOMEM;
	}

	strcpy(s3c_ipc->name, S3C_IPC_NAME);

	return 0;
}

/*
static void s3c_ipc_reset(void)
{
	u32 cfg;

	cfg = readl(s3c_ipc->regs + S3C_IPC_CONTROL);
	cfg |= S3C_IPC_CONTROL_RESET;
	writel(cfg, s3c_ipc->regs + S3C_IPC_CONTROL);
}
*/

void shadow_update(void)
{
	writel(S3C_IPC_SHADOW_UPDATE_ENABLE, s3c_ipc->regs + S3C_IPC_SHADOW_UPDATE);
}

void ipc_setpostprocessing_onoff(u32 onoff)
{
	u32 cfg;

	cfg = readl(s3c_ipc->regs + S3C_IPC_BYPASS);
	if (!onoff)
		cfg |= S3C_IPC_PP_BYPASS_DISABLE; // 1
	else
		cfg &= S3C_IPC_PP_BYPASS_ENABLE; // 0
	
	writel(cfg, s3c_ipc->regs + S3C_IPC_BYPASS);

	shadow_update();

}

void ipc_enable_ip(u32 onoff)
{
	u32 cfg;

	cfg = readl(s3c_ipc->regs + S3C_IPC_ENABLE);
	if (!onoff)
		cfg &= S3C_IPC_OFF;
	else
		cfg |= S3C_IPC_ON;
		
	writel(cfg, s3c_ipc->regs + S3C_IPC_ENABLE);
	
}

void ipc_swreset(void)
{
	u32 cfg;

	do 
	{
		cfg = readl(s3c_ipc->regs + S3C_IPC_SRESET);
	} while ((cfg & S3C_IPC_SRESET_MASK) != 0); 
		
	writel(S3C_IPC_SRESET_ENABLE, s3c_ipc->regs + S3C_IPC_SRESET);
}

void ipc_on(void)
{
	ipc_setpostprocessing_onoff(ON);
	ipc_enable_ip(ON);
}

void ipc_off(void)
{
	ipc_setpostprocessing_onoff(OFF);
	ipc_enable_ip(OFF);
	ipc_swreset();
}

void ipc_field_id_control(ipc_field_id id)
{
	writel(id, s3c_ipc->regs + S3C_IPC_FIELD_ID);
	shadow_update();
}

void ipc_field_id_mode(ipc_field_id_sel sel, ipc_field_id_togl toggle)
{
	u32 cfg;

	cfg = readl(s3c_ipc->regs + S3C_IPC_MODE);
	cfg |= S3C_IPC_FIELD_ID_SELECTION(sel);
	writel(cfg, s3c_ipc->regs + S3C_IPC_MODE);	

	cfg = readl(s3c_ipc->regs + S3C_IPC_MODE);
	cfg |= S3C_IPC_FIELD_ID_AUTO_TOGGLING(toggle);
	writel(cfg, s3c_ipc->regs + S3C_IPC_MODE);

	shadow_update();	
}

void ipc_2d_enable(ipc_enoff onoff)
{
	u32 cfg;

	cfg = readl(s3c_ipc->regs + S3C_IPC_MODE);
	cfg &= ~S3C_IPC_2D_MASK;
	cfg |= S3C_IPC_2D_CTRL(onoff);
	writel(cfg, s3c_ipc->regs + S3C_IPC_MODE);

	shadow_update();
}

void ipc_setmode_and_imgsize(ipc_source ipc_src, ipc_destination ipc_dst, ipc_controlvariable ipc_convar)
{
	u32 v_ratio, h_ratio;

	ipc_field_id_control(IPC_BOTTOM_FIELD);
	ipc_field_id_mode(CAM_FIELD_SIG, AUTO);
	ipc_2d_enable(ipc_convar.modeval);	// Enalbed : 2D IPC , Disabled : Horizon Double Scailing

	writel(S3C_IPC_SRC_WIDTH_SET(ipc_src.srchsz), s3c_ipc->regs + S3C_IPC_SRC_WIDTH);
	writel(S3C_IPC_SRC_HEIGHT_SET(ipc_src.srcvsz), s3c_ipc->regs + S3C_IPC_SRC_HEIGHT);

	writel(S3C_IPC_DST_WIDTH_SET(ipc_dst.dsthsz), s3c_ipc->regs + S3C_IPC_DST_WIDTH);
	writel(S3C_IPC_DST_HEIGHT_SET(ipc_dst.dstvsz), s3c_ipc->regs + S3C_IPC_DST_HEIGHT);

	if (1 == ipc_convar.modeval)
		h_ratio = IPC_2D_ENABLE;
	else
		h_ratio = IPC_HOR_SCALING_ENABLE;		

	writel(h_ratio, s3c_ipc->regs + S3C_IPC_H_RATIO);

	v_ratio = IPC_2D_ENABLE;
	writel(v_ratio, s3c_ipc->regs + S3C_IPC_V_RATIO);
	
	shadow_update();	
}

void ipc_init_enhancing_parameter(void)
{
	u32 i;
	
	for(i = 0; i < 8; i++)
	{
		ipc_enh_var.brightness[i] = 0x0;
		ipc_enh_var.contrast[i] = 0x80;
	}

	ipc_enh_var.saturation = 0x80;
	ipc_enh_var.sharpness = NO_EFFECT;
	ipc_enh_var.thhnoise = 0x5;
	ipc_enh_var.brightoffset = 0x0;
}

void ipc_setcontrast(u32 *contrast)
{
	u32 i, line_eq[8];

	for(i = 0; i < 8; i++)
	{
		line_eq[i] = readl(s3c_ipc->regs + (S3C_IPC_PP_LINE_EQ0 + 4 * i)); 

		line_eq[i] &= ~(S3C_IPC_PP_LINE_CONTRAST_MASK << 0);
		line_eq[i] |= ((contrast[i] & S3C_IPC_PP_LINE_CONTRAST_MASK) << 0);

		writel(line_eq[i], s3c_ipc->regs + (S3C_IPC_PP_LINE_EQ0 + 4 * i));
	}

	shadow_update();
}

void ipc_setbrightness(u32 *brightness)
{
	u32 i, line_eq[8];

	for(i = 0; i < 8; i++)
	{
		line_eq[i] = readl(s3c_ipc->regs + (S3C_IPC_PP_LINE_EQ0 + 4 * i)); 

		line_eq[i] &= ~(S3C_IPC_PP_LINE_BRIGTHNESS_MASK << 8);
		line_eq[i] |= ((brightness[i] & S3C_IPC_PP_LINE_BRIGTHNESS_MASK) << 8);

		writel(line_eq[i], s3c_ipc->regs + (S3C_IPC_PP_LINE_EQ0 + 4 * i));
	}

	shadow_update();
}

void ipc_setbright_offset(u32 offset)
{
	writel(S3C_IPC_PP_BRIGHT_OFFSET_SET(offset), s3c_ipc->regs + S3C_IPC_PP_BRIGHT_OFFSET);
	shadow_update();
}

void ipc_setsaturation(u32 saturation)
{
	writel(S3C_IPC_PP_SATURATION_SET(saturation), s3c_ipc->regs + S3C_IPC_PP_SATURATION);
	shadow_update();
}

void ipc_setsharpness(ipc_sharpness Sharpness, u32 threshold)
{
	u32 sharpval;

	switch (Sharpness) {
	case NO_EFFECT:
		sharpval = (0x0<<0);
		break;
	case MIN_EDGE:
		sharpval = (0x1<<0);
		break;
	case MODERATE_EDGE:
		sharpval = (0x2<<0);
		break;
	default:
		sharpval = (0x3<<0);
		break;		
	}

	writel(S3C_IPC_PP_TH_HNOISE_SET(threshold) | sharpval, s3c_ipc->regs + S3C_IPC_PP_SHARPNESS);

	shadow_update();	
}

void ipc_setpolyphase_filter(volatile u32 filter_reg, const s8* filter_coef, u16 tap_sz)
{
	u32 i,j;
	u8* temp_filter_coef;
	u16 temp_tap_sz;
	volatile u32 filter_baseaddr;
	
	filter_baseaddr = filter_reg;
	temp_filter_coef = (u8*)filter_coef;

	for (i = 0; i < tap_sz; i++)
	{
		// VP_POLYx_Y0/1(/2/3)_xx Setting
		temp_tap_sz = tap_sz - i - 1;
		
		for (j = 0; j < 4; j++)
		{
			// VP_POLYx_Yx_LL/LH/HL/HH Setting
			writel(( (temp_filter_coef[4*j*tap_sz + temp_tap_sz] << 24)	\
				| (temp_filter_coef[(4*j + 1)*tap_sz + temp_tap_sz] << 16)	\
				| (temp_filter_coef[(4*j + 2)*tap_sz + temp_tap_sz] << 8) 
				| (temp_filter_coef[(4*j + 3)*tap_sz + temp_tap_sz]) ),filter_baseaddr);
			filter_baseaddr += 4;
		}
	}
}


void ipc_setpolyphase_filterset(ipc_filter_h_pp h_filter, ipc_filter_v_pp v_filter)
{
	ipc_setpolyphase_filter(IPC_POLY8_Y0_LL, sIpc8tapCoef_Y_H + h_filter*16*8, 8);
	// Horizontal C 4-tap
	ipc_setpolyphase_filter(IPC_POLY4_C0_LL, sIpc4tapCoef_C_H + h_filter*16*4, 4);
	// Vertical Y 4-tap
	ipc_setpolyphase_filter(IPC_POLY4_Y0_LL, sIpc4tapCoef_Y_V + v_filter*16*4, 4);
}

void ipc_setfilter(void)
{

	u32 h_ratio, v_ratio;
	ipc_filter_h_pp h_filter;
	ipc_filter_v_pp v_filter;


	h_ratio = readl(s3c_ipc->regs + S3C_IPC_H_RATIO);
	v_ratio = readl(s3c_ipc->regs + S3C_IPC_V_RATIO);


	// 	For the real interlace mode, the vertical ratio should be used after divided by 2.
	//	Because in the interlace mode, all the IPC output is used for FIMD display
	//	and it should be the same as one field of the progressive mode. 
	//	Therefore the same filter coefficients should be used for the same the final output video.
	//	When half of the interlace V_RATIO is same as the progressive V_RATIO,
	//	the final output video scale is same. (20051104,ishan)

	//Horizontal Y 8tap 
	//Horizontal C 4tap	
	
	if (h_ratio <= (0x1<<16))			// 720->720 or zoom in
		h_filter = IPC_PP_H_NORMAL;
	else if (h_ratio <= (0x9<<13))		// 720->640
		h_filter = IPC_PP_H_8_9 ;
	else if(h_ratio <= (0x1<<17))		// 2->1
		h_filter = IPC_PP_H_1_2;
	else if(h_ratio <= (0x3<<16))		// 2->1	
		h_filter = IPC_PP_H_1_3;
	else // 0x800						// 4->1
		h_filter = IPC_PP_H_1_4;

	// Vertical Y 4tap
	if (v_ratio <= (0x1<<16))			// 720->720 or zoom in
		v_filter = IPC_PP_V_NORMAL;
	else if (v_ratio <= (0x3<<15))		//*6->5
		v_filter = IPC_PP_V_5_6;
	else if(v_ratio <= (0x5<<14))		// 4->3
		v_filter = IPC_PP_V_3_4;
	else if (v_ratio <= (0x1<<17))		// 2->1
		v_filter = IPC_PP_V_1_2;
	else if (v_ratio <= (0x3<<16))		// 3->1
		v_filter = IPC_PP_V_1_3;
	else // 0x400						// 4->1
		v_filter = IPC_PP_V_1_4;

	ipc_setpolyphase_filterset(h_filter, v_filter);
	
}

void IPC_IPC_VERSION_INFO(void)
{
	u32 ver = 0;
	ver = readl(s3c_ipc->regs + S3C_IPC_VERSION_INFO);	
}

void ipc_init(ipc_source ipc_src, ipc_destination ipc_dst, ipc_controlvariable ipc_convar)
{
		 
	ipc_swreset();
	ipc_enable_ip(OFF);

	ipc_setpostprocessing_onoff(OFF);
		
	//Initialize the Image Information
	ipc_setmode_and_imgsize(ipc_src, ipc_dst, ipc_convar);

	//Initialize Enhancement 
	ipc_init_enhancing_parameter();
	ipc_setcontrast(ipc_enh_var.contrast); 
	ipc_setbrightness(ipc_enh_var.brightness);
	ipc_setbright_offset(ipc_enh_var.brightoffset);
	ipc_setsaturation(ipc_enh_var.saturation);
	ipc_setsharpness(ipc_enh_var.sharpness, ipc_enh_var.thhnoise);

	//Initialize the Poly-Phase Filter Coefficients
	ipc_setfilter();

	writel(S3C_IPC_PEL_RATE_SET, s3c_ipc->regs + S3C_IPC_PEL_RATE_CTRL);

	shadow_update();
}

int ipc_initip(u32 input_width, u32 input_height,  ipc_2d ipc2d)
{
	if(input_width > IN_SC_MAX_WIDTH || input_height > IN_SC_MAX_HEIGHT) {
		err("IPC input size error\n");
		ipc_off();
		return -EINVAL;
	}

	ipc_input_src.imghsz = input_width;
	ipc_input_src.imgvsz = input_height;
	ipc_input_src.srchsz = input_width;
	ipc_input_src.srcvsz = input_height;

	ipc_output_dst.scanmode = PROGRESSIVE;
			
	if(ipc2d == IPC_2D) {
		ipc_output_dst.dsthsz = input_width;
		ipc_output_dst.dstvsz = input_height*2;		
	} else {
		ipc_output_dst.dsthsz = input_width*2;
		ipc_output_dst.dstvsz = input_height;		
	}

	ipc_con_var.modeval = ipc2d;

	ipc_init(ipc_input_src, ipc_output_dst, ipc_con_var);

	return 0;
}

static void s3c_ipc_stop(struct platform_device *pdev)
{
	ipc_off();
}

static int s3c_ipc_probe(struct platform_device *pdev)
{
	struct s3c_platform_ipc *pdata;
	struct resource *res;

	s3c_ipc_set_info();

	pdata = to_ipc_plat(&pdev->dev);
	if (pdata->cfg_gpio)
		pdata->cfg_gpio(pdev);
/*
	s3c_ipc->clock = clk_get(&pdev->dev, pdata->clk_name);
	if (IS_ERR(s3c_ipc->clock)) {
		err("failed to get ipc clock source\n");
		return -EINVAL;
	}

	clk_enable(s3c_ipc->clock);
*/
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		err("failed to get io memory region\n");
		return -EINVAL;
	}

	res = request_mem_region(res->start, res->end - res->start + 1, pdev->name);
	if (!res) {
		err("failed to request io memory region\n");
		return -EINVAL;
	}

	/* ioremap for register block */
	s3c_ipc->regs = ioremap(res->start, res->end - res->start + 1);
	if (!s3c_ipc->regs) {
		err("failed to remap io region\n");
		return -EINVAL;
	}


	return 0;	
}

static int s3c_ipc_remove(struct platform_device *pdev)
{
	s3c_ipc_stop(pdev);
	kfree(s3c_ipc);

	return 0;
}

int s3c_ipc_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

int s3c_ipc_resume(struct platform_device *dev)
{
	return 0;
}

static struct platform_driver s3c_ipc_driver = {
	.probe		= s3c_ipc_probe,
	.remove		= s3c_ipc_remove,
	.suspend	= s3c_ipc_suspend,
	.resume		= s3c_ipc_resume,
	.driver		= {
		.name	= "s3c-ipc",
		.owner	= THIS_MODULE,
	},
};

static int s3c_ipc_register(void)
{
	platform_driver_register(&s3c_ipc_driver);

	return 0;
}

static void s3c_ipc_unregister(void)
{
	platform_driver_unregister(&s3c_ipc_driver);
}

module_init(s3c_ipc_register);
module_exit(s3c_ipc_unregister);
	
MODULE_AUTHOR("Youngmok, Song <ym.song@samsung.com>");
MODULE_DESCRIPTION("IPC support for FIMC driver");
MODULE_LICENSE("GPL");

