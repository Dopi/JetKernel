/* linux/drivers/media/video/samsung/tv20/s5p_tv.h
 *
 * TV out driver header file for Samsung TVOut driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/videodev2.h>
#include <linux/platform_device.h>

#ifdef CONFIG_CPU_S5PC110
#include "s5pc110/tv_out_s5pc110.h"
#endif

#ifdef CONFIG_CPU_S5PC100
#include "s5pc100/tv_out_s5pc100.h"
#endif

//#define COFIG_TVOUT_DBG


#define S5P_TVMAX_CTRLS 	3

#ifdef CONFIG_CPU_S5PC100
#define FIX_27M_UNSTABLE_ISSUE
#endif

#define V4L2_STD_ALL_HD		((v4l2_std_id)0xffffffff)

#define S5P_HDCP_I2C_ADDR		0x74
#define I2C_DRIVERID_S5P_HDCP		510

#ifdef CONFIG_JUPITER //??

#define S5P_USBSW_I2C_ADDR 0x25

#endif




#define TVOUT_MINOR_VIDEO		14	// TVOUT VIDEO OUT
#define TVOUT_MINOR_GRP0		21	// TVOUT OVERLAY0
#define TVOUT_MINOR_GRP1		22	// TVOUT OVERLAY1

#define USE_VMIXER_INTERRUPT		1

// AVI InfoFrame
#define AVI_RGB_IF		(0x0<<5)
#define AVI_YCBCR444_IF		(0x2<<5)

#define AVI_ITU601		(0x1<<6)
#define AVI_ITU709		(0x2<<6)

#define AVI_PAR_4_3		(0x1<<4)
#define AVI_PAR_16_9		(0x2<<4)

#define AVI_NO_PIXEL_REPEAT	(0x0<<0)

#define AVI_VIC_2		(2<<0) 	// 720x480p@59.94/60Hz 4:3
#define AVI_VIC_3		(3<<0) 	// 720x480p@59.94/60Hz 16:9
#define AVI_VIC_4		(4<<0) 	// 1280x720p@59.94/60Hz 16:9
#define AVI_VIC_5		(5<<0) 	// 1920x1080i@59.94/60Hz 16:9
#define AVI_VIC_16		(16<<0) // 1920x1080p@60Hz 16:9
#define AVI_VIC_17		(17<<0) // 720x576p@50Hz 4:3
#define AVI_VIC_18		(18<<0) // 720x576p@50Hz 16:9
#define AVI_VIC_19		(19<<0) // 1280x720p@50Hz 16:9
#define AVI_VIC_20		(20<<0) // 1920x1080i@50Hz 16:9
#define AVI_VIC_31		(31<<0) // 1920x1080p@50Hz

#define VP_UPDATE_RETRY_MAXIMUM 	30
#define VP_WAIT_UPDATE_SLEEP 		3

typedef struct _tvout_output_if {
	s5p_tv_disp_mode	disp_mode;
	s5p_tv_o_mode	out_mode;
} tvout_output_if;

typedef struct _s5p_img_size {
	u32 img_width;
	u32 img_height;
}s5p_img_size;

typedef struct _s5p_img_offset {
	u32 offset_x;
	u32 offset_y;
}s5p_img_offset;

/*
* Video Layer Data
*/
typedef struct _s5p_video_img_address {
	u32 y_address;
	u32 c_address;
}s5p_video_img_address;

typedef struct _s5p_vl_mode {
	bool line_skip;
	s5p_vp_mem_mode mem_mode;
	s5p_vp_chroma_expansion chroma_exp;
	s5p_vp_filed_id_toggle toggle_id;
}s5p_vl_mode;

typedef struct _s5p_vl_sharpness {
	u32 th_noise;
	s5p_vp_sharpness_control sharpness;
}s5p_vl_sharpness;

typedef struct _s5p_vl_csc_ctrl {
	bool sub_y_offset_en;
	bool csc_en;
}s5p_vl_csc_ctrl;

typedef struct _s5p_video_poly_filter_coef {
	s5p_vp_poly_coeff poly_coeff;
	signed char ch0;
	signed char ch1;
	signed char ch2;
	signed char ch3;
}s5p_video_poly_filter_coef;

typedef struct _s5p_vl_bright_contrast_ctrl {
	s5p_vp_line_eq eq_num;
	u32 intc;
	u32 slope;
}s5p_vl_bright_contrast_ctrl;

typedef struct _s5p_video_csc_coef {
	s5p_vp_csc_coeff csc_coeff;
	u32 coeff;
}s5p_video_csc_coef;

typedef struct _s5p_vl_param {
	bool win_blending;
	u32 alpha;
	u32 priority;
	u32 top_y_address;
	u32 top_c_address;
	s5p_endian_type src_img_endian;
	u32 img_width;
	u32 img_height;
	u32 src_offset_x;
	u32 src_offset_y;
	u32 src_width;
	u32 src_height;
	u32 dest_offset_x;
	u32 dest_offset_y;
	u32 dest_width;
	u32 dest_height;
}s5p_vl_param;


/*
* GRP Layer Data
*/
typedef struct _s5p_tv_vo {
	u32 index; 			
	struct v4l2_framebuffer fb; 	
	struct v4l2_window win;  	
	struct v4l2_rect dst_rect;	
	bool win_blending;
	bool blank_change;
	bool pixel_blending;
	bool pre_mul;
	u32 blank_color;
	u32 priority;
	u32 base_addr;
}s5p_tv_vo;

/*
*BG Layer Data
*/
typedef struct _s5p_bg_dither {
	bool cr_dither_en;
	bool cb_dither_en;
	bool y_dither_en;
}s5p_bg_dither;


typedef struct _s5p_bg_color {
	u32 color_y;
	u32 color_cb;
	u32 color_cr;
}s5p_bg_color;

/* 
* Video Mixer Data
*/
typedef struct _s5p_vm_csc_coef {
	s5p_yuv_fmt_component component;
	s5p_tv_coef_y_mode mode;
	u32 coeff_0;
	u32 coeff_1;
	u32 coeff_2;
}s5p_vm_csc_coef;

/*
* SDout Data
*/
typedef struct _s5p_sdout_order {
	s5p_sd_order order;
	bool dac[3];
}s5p_sdout_order;

typedef struct _s5p_sd_vscale_cfg {
	s5p_sd_level component_level;
	s5p_sd_vsync_ratio component_ratio;
	s5p_sd_level composite_level;
	s5p_sd_vsync_ratio composite_ratio;
}s5p_sd_vscale_cfg;

typedef struct _s5p_sd_vbi {
	bool wss_cvbs;
	s5p_sd_closed_caption_type caption_cvbs;
	bool wss_y_svideo;
	s5p_sd_closed_caption_type caption_y_svideo;
	bool cgmsa_rgb;
	bool wss_rgb;
	s5p_sd_closed_caption_type caption_rgb;
	bool cgmsa_y_pb_pr;
	bool wss_y_pb_pr;
	s5p_sd_closed_caption_type caption_y_pb_pr;
}s5p_sd_vbi;

typedef struct _s5p_sd_offset_gain {
	s5p_sd_channel_sel channel;
	u32 offset;
	u32 gain;
}s5p_sd_offset_gain;

typedef struct _s5p_sd_delay {
	u32 delay_y;
	u32 offset_video_start;
	u32 offset_video_end;
}s5p_sd_delay;

typedef struct _s5p_sd_bright_hue_saturat {
	bool bright_hue_sat_adj;
	u32 gain_brightness;
	u32 offset_brightness;
	u32 gain0_cb_hue_saturation;
	u32 gain1_cb_hue_saturation;
	u32 gain0_cr_hue_saturation;
	u32 gain1_cr_hue_saturation;
	u32 offset_cb_hue_saturation;
	u32 offset_cr_hue_saturation;
}s5p_sd_bright_hue_saturat;

typedef struct _s5p_sd_rgb_compensat {
	bool rgb_color_compensation;
	u32 max_rgb_cube;
	u32 min_rgb_cube;
}s5p_sd_rgb_compensat;

typedef struct _s5p_sd_cvbs_compensat {
	bool cvbs_color_compensation;
	u32 y_lower_mid;
	u32 y_bottom;
	u32 y_top;
	u32 y_upper_mid;
	u32 radius;
}s5p_sd_cvbs_compensat;

typedef struct _s5p_sd_svideo_compensat {
	bool y_color_compensation;
	u32 y_top;
	u32 y_bottom;
	u32 yc_cylinder;
}s5p_sd_svideo_compensat;

typedef struct _s5p_sd_component_porch {
	u32 back_525;
	u32 front_525;
	u32 back_625;
	u32 front_625;
}s5p_sd_component_porch;

typedef struct _s5p_sd_vesa_rgb_sync {
	s5p_sd_vesa_rgb_sync_type sync_type;
	s5p_tv_active_polarity vsync_active;
	s5p_tv_active_polarity hsync_active;
}s5p_sd_vesa_rgb_sync;

typedef struct _s5p_sd_ch_xtalk_cancellat_coeff {
	s5p_sd_channel_sel channel;
	u32 coeff1;
	u32 coeff2;
}s5p_sd_ch_xtalk_cancellat_coeff;

typedef struct _s5p_sd_closed_caption {
	u32 display_cc;
	u32 nondisplay_cc;
}s5p_sd_closed_caption;

typedef struct _s5p_sd_525_data {
	bool analog_on;	
	s5p_sd_525_copy_permit copy_permit;
	s5p_sd_525_mv_psp mv_psp;
	s5p_sd_525_copy_info copy_info;
	s5p_sd_525_aspect_ratio display_ratio;
}s5p_sd_525_data;

typedef struct _s5p_sd_625_data {
	bool surroun_f_sound;
	bool copyright;
	bool copy_protection;
	bool text_subtitles;
	s5p_sd_625_subtitles open_subtitles;
	s5p_sd_625_camera_film camera_film;
	s5p_sd_625_color_encoding color_encoding;
	bool helper_signal;
	s5p_sd_625_aspect_ratio display_ratio;
}s5p_sd_625_data;

/*
* HDMI video Data
*/
typedef struct _s5p_hdmi_bluescreen {
	bool enable;
	u8 cb_b;
	u8 y_g;
	u8 cr_r;
}s5p_hdmi_bluescreen;

typedef struct _s5p_hdmi_color_range {
	u8 y_min;
	u8 y_max;
	u8 c_min;
	u8 c_max;
}s5p_hdmi_color_range;

typedef struct _s5p_hdmi_video_infoframe {
	s5p_hdmi_transmit trans_type;
	u8 check_sum;
	u8 *data;
}s5p_hdmi_video_infoframe;

typedef struct _s5p_hdmi_tg_cmd {
	bool timing_correction_en;
	bool bt656_sync_en;
	bool tg_en;
}s5p_hdmi_tg_cmd;

typedef struct _s5p_hdmi_spd_infoframe {
	s5p_hdmi_transmit trans_type;
	u8 *spd_header;
	u8 *spd_data;
}s5p_hdmi_spd_infoframe;

/*
* TVout API Data
*/
typedef struct s5p_tv_v4l2 {
	struct v4l2_output      *output;
	struct v4l2_standard	*std;
	struct v4l2_format	*fmt_v;
	struct v4l2_format	*fmt_vo_0;
	struct v4l2_format	*fmt_vo_1;
} s5p_tv_v4l2_t;


typedef struct _s5p_tv_status {
	// TVOUT_SET_INTERFACE_PARAM
	bool tvout_param_available;
	tvout_output_if tvout_param;

	// TVOUT_SET_OUTPUT_ENABLE/DISABLE
	bool tvout_output_enable;
	bool videoplaying; //mkh



	// TVOUT_SET_LAYER_MODE/POSITION
	bool vl_mode;
	bool grp_mode[2];

	// Video Layer Parameters
	s5p_vl_param vl_basic_param;
	s5p_vl_mode vl_op_mode;
	s5p_vl_sharpness vl_sharpness;
	s5p_vl_csc_ctrl vl_csc_control;
	s5p_vl_bright_contrast_ctrl vl_bc_control[8];

	s5p_vp_src_color src_color;
	s5p_vp_field field_id;
	s5p_vp_pxl_rate vl_rate;
	s5p_vp_csc_type vl_csc_type;

	u32 vl_top_y_address;
	u32 vl_top_c_address;
	u32 vl_bottom_y_address;
	u32 vl_bottom_c_address;
	u32 vl_src_offset_x;
	u32 vl_src_x_fact_step;
	u32 vl_src_offset_y;
	u32 vl_src_width;
	u32 vl_src_height;
	u32 vl_dest_offset_x;
	u32 vl_dest_offset_y;
	u32 vl_dest_width;
	u32 vl_dest_height;
	bool vl2d_ipc;

	bool vl_poly_filter_default;
	bool vl_bypass_post_process;
	u32 vl_saturation;
	bool us_vl_brightness;
	u8 vl_contrast;
	u32 vl_bright_offset;
	bool vl_csc_coef_default;

	// GRP Layer Common Parameters
	s5p_vmx_burst_mode grp_burst;
	s5p_endian_type grp_endian;

	// BackGroung Layer Parameters
	s5p_bg_dither bg_dither;
	s5p_bg_color bg_color[3];

	// Video Mixer Parameters
	bool vm_csc_coeff_default;

	// SDout Parameters
	s5p_sd_vscale_cfg sdout_video_scale_cfg;
	s5p_sd_vbi sdout_vbi;
	s5p_sd_offset_gain sdout_offset_gain[3];
	s5p_sd_delay sdout_delay;
	s5p_sd_bright_hue_saturat sdout_bri_hue_set;
	s5p_sd_rgb_compensat sdout_rgb_compen;
	s5p_sd_cvbs_compensat sdout_cvbs_compen;
	s5p_sd_svideo_compensat sdout_svideo_compen;
	s5p_sd_component_porch sdout_comp_porch;
	s5p_sd_vesa_rgb_sync sdout_rgb_sync;
	s5p_sd_ch_xtalk_cancellat_coeff sdout_xtalk_cc[3];
	s5p_sd_closed_caption sdout_closed_capt;
	s5p_sd_525_data sdout_wss_525;
	s5p_sd_625_data sdout_wss_625;
	s5p_sd_525_data sdout_cgms_525;
	s5p_sd_625_data sdout_cgms_625;

	s5p_sd_order sdout_order;
	s5p_sd_sync_sig_pin sdout_sync_pin;

	bool sdout_color_sub_carrier_phase_adj;
	bool sdout_dac_on[3];
	s5p_sd_macrovision_val sdout_macrovision;

	bool sdout_y_pb_pr_comp;

	// HDMI video parameters
	s5p_hdmi_bluescreen hdmi_video_blue_screen;
	s5p_hdmi_color_range hdmi_color_range;
	s5p_hdmi_video_infoframe hdmi_av_info_frame;
	s5p_hdmi_video_infoframe hdmi_mpg_info_frame;
	s5p_hdmi_tg_cmd hdmi_tg_cmd;
	u8 avi_byte[13];
	u8 mpg_byte[5];

	// HDMI parameters
	s5p_hdmi_spd_infoframe hdmi_spd_info_frame;
	u8 spd_header[3];
	u8 spd_data[28];
	bool hdcp_en;
	s5p_hdmi_audio_type hdmi_audio_type;
	bool hpd_status;

	// TVOUT_SET_LAYER_ENABLE/DISABLE
	bool vp_layer_enable;
	bool grp_layer_enable[2];

	// i2c for hdcp port

	struct i2c_client 	*hdcp_i2c_client;

	s5p_tv_vo		overlay[2];

	struct video_device      *video_dev[3];

	struct clk *tvenc_clk;
	struct clk *vp_clk;
	struct clk *mixer_clk;
	struct clk *hdmi_clk;
	struct clk *sclk_tv;
	struct clk *sclk_hdmi;
	struct clk *sclk_mixer;
	
	s5p_tv_v4l2_t 	v4l2;
} s5p_tv_status;


/*
* V4L2 TVOUT EXTENSIONS
*
*/
// Input
#define V4L2_INPUT_TYPE_MSDMA		3
#define V4L2_INPUT_TYPE_FIFO		4

// Output
#define V4L2_OUTPUT_TYPE_MSDMA		4
#define V4L2_OUTPUT_TYPE_COMPOSITE	5
#define V4L2_OUTPUT_TYPE_SVIDEO		6
#define V4L2_OUTPUT_TYPE_YPBPR_INERLACED	7
#define V4L2_OUTPUT_TYPE_YPBPR_PROGRESSIVE	8
#define V4L2_OUTPUT_TYPE_RGB_PROGRESSIVE	9
#define V4L2_OUTPUT_TYPE_DIGITAL		10

// STD
#define V4L2_STD_PAL_BDGHI 	V4L2_STD_PAL_B|	\
				V4L2_STD_PAL_D|	\
				V4L2_STD_PAL_G|	\
				V4L2_STD_PAL_H|	\
				V4L2_STD_PAL_I

#define V4L2_STD_480P_60_16_9	((v4l2_std_id)0x04000000)
#define V4L2_STD_480P_60_4_3	((v4l2_std_id)0x05000000)
#define V4L2_STD_576P_50_16_9	((v4l2_std_id)0x06000000)
#define V4L2_STD_576P_50_4_3	((v4l2_std_id)0x07000000)
#define V4L2_STD_720P_60	((v4l2_std_id)0x08000000)
#define V4L2_STD_720P_50	((v4l2_std_id)0x09000000)
#define V4L2_STD_1080P_60	((v4l2_std_id)0x0a000000)
#define V4L2_STD_1080P_50	((v4l2_std_id)0x0b000000)

#define FORMAT_FLAGS_DITHER       	0x01
#define FORMAT_FLAGS_PACKED       	0x02
#define FORMAT_FLAGS_PLANAR       	0x04
#define FORMAT_FLAGS_RAW         	0x08
#define FORMAT_FLAGS_CrCb         	0x10

// ext. param
#define V4L2_FBUF_FLAG_PRE_MULTIPLY	0x0040
#define V4L2_FBUF_CAP_PRE_MULTIPLY	0x0080

struct v4l2_window_s5p_tvout {
	u32	capability;
	u32	flags;
	u32	priority;

	struct v4l2_window	win;
};

struct v4l2_pix_format_s5p_tvout {
	void *base_y;
	void *base_c;
	bool src_img_endian;

	struct v4l2_pix_format	pix_fmt;
};

extern const struct v4l2_ioctl_ops s5p_tv_v4l2_v_ops;
extern const struct v4l2_ioctl_ops s5p_tv_v4l2_vo_ops;

extern void s5p_tv_v4l2_init_param(void);
extern int s5p_tv_v_ioctl(struct file *file, u32 cmd, unsigned long arg);
extern int s5p_tv_vo_ioctl(struct file *file, u32 cmd, unsigned long arg);

/*
 * STDA layer api - must be refine!!
 *
*/

#ifdef CONFIG_CPU_S5PC100
int __init __s5p_hdmi_probe(struct platform_device *pdev, u32 res_num);
#endif

#ifdef CONFIG_CPU_S5PC110
int __init __s5p_hdmi_probe(struct platform_device *pdev, u32 res_num, u32 res_num2);
#endif

int __init __s5p_sdout_probe(struct platform_device *pdev, u32 res_num);
int __init __s5p_mixer_probe(struct platform_device *pdev, u32 res_num);
int __init __s5p_vp_probe(struct platform_device *pdev, u32 res_num);
int __init __s5p_tvclk_probe(struct platform_device *pdev, u32 res_num);

int __init __s5p_hdmi_release(struct platform_device *pdev);
int __init __s5p_sdout_release(struct platform_device *pdev);
int __init __s5p_mixer_release(struct platform_device *pdev);
int __init __s5p_vp_release(struct platform_device *pdev);
int __init __s5p_tvclk_release(struct platform_device *pdev);



// HDMI
extern	bool _s5p_hdmi_api_proc(unsigned long arg, u32 cmd);
extern	bool _s5p_hdmi_video_api_proc(unsigned long arg, u32 cmd);

// GRP
extern	bool _s5p_grp_api_proc(unsigned long arg, u32 cmd);
extern 	bool _s5p_grp_init_param(s5p_tv_vmx_layer vm_layer, unsigned long p_buf_in);
extern	bool _s5p_grp_start(s5p_tv_vmx_layer vmLayer);
extern	bool _s5p_grp_stop(s5p_tv_vmx_layer vmLayer);

// TVOUT_IF
extern	bool _s5p_tv_if_api_proc(unsigned long arg, u32 cmd);
extern	bool _s5p_tv_if_init_param(void);
extern	bool _s5p_tv_if_start(void);
extern	bool _s5p_tv_if_stop(void);
extern	bool _s5p_tv_if_set_disp(void);

extern	bool _s5p_bg_api_proc(unsigned long arg, u32 cmd);
extern	bool _s5p_sdout_api_proc(unsigned long arg, u32 cmd);

// Video Layer
extern	bool _s5p_vlayer_set_blending(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_alpha(unsigned long p_buf_in);
extern	bool _s5p_vlayer_api_proc(unsigned long arg, u32 cmd);
extern	bool _s5p_vlayer_init_param(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_priority(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_field_id(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_top_address(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_bottom_address(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_img_size(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_src_position(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_dest_position(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_src_size(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_dest_size(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_brightness(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_contrast(unsigned long p_buf_in);
extern	void _s5p_vlayer_get_priority(unsigned long p_buf_out);
extern	bool _s5p_vlayer_set_brightness_contrast_control(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_poly_filter_coef(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_csc_coef(unsigned long p_buf_in);
extern	bool _s5p_vlayer_start(void);
extern	bool _s5p_vlayer_stop(void);

/*
 * raw i/o ftn!!
 *
*/
// HDCP
void 	__s5p_read_hdcp_data(u8 reg_addr, u8 count, u8 *data);
void 	__s5p_write_hdcp_data(u8 reg_addr, u8 count, u8 *data);
void 	__s5p_write_ainfo(void);
void 	__s5p_write_an(void);
void 	__s5p_write_aksv(void);
void 	__s5p_read_bcaps(void);
void 	__s5p_read_bksv(void);
bool 	__s5p_compare_p_value(void);
bool 	__s5p_compare_r_value(void);
void 	__s5p_reset_authentication(void);
void 	__s5p_make_aes_key(void);
void 	__s5p_set_av_mute_on_off(u32 on_off);
void 	__s5p_start_encryption(void);
void 	__s5p_start_decrypting(const u8 *hdcp_key, u32 hdcp_key_size);
bool 	__s5p_check_repeater(void);
bool 	__s5p_is_occurred_hdcp_event(void);
irqreturn_t __s5p_hdmi_irq(int irq, void *dev_id);
bool 	__s5p_is_decrypting_done(void);
void 	__s5p_set_hpd_detection(u32 detection_type, bool hdcp_enabled, struct i2c_client *client);
bool 	__s5p_start_hdcp(struct i2c_client *ddc_port);
void __s5p_stop_hdcp(void);
void __s5p_hdcp_reset(void);

// HDMI
void 	__s5p_hdmi_set_hpd_onoff(bool on_off);
void 	__s5p_hdmi_audio_set_config(s5p_tv_audio_codec_type audio_codec);
void 	__s5p_hdmi_audio_set_acr(u32 sample_rate);
void 	__s5p_hdmi_audio_set_asp(void);
void 	__s5p_hdmi_audio_clock_enable(void);
void 	__s5p_hdmi_audio_set_repetition_time(s5p_tv_audio_codec_type audio_codec, u32 bits, u32 frame_size_code);
void 	__s5p_hdmi_audio_irq_enable(u32 irq_en);
void 	__s5p_hdmi_audio_set_aui(s5p_tv_audio_codec_type audio_codec, u32 sample_rate, u32 bits);
void 	__s5p_hdmi_video_set_bluescreen(bool en, u8 cb, u8 y_g, u8 cr_r);
s5p_tv_hdmi_err __s5p_hdmi_init_spd_infoframe(s5p_hdmi_transmit trans_type, u8 *spd_header, u8 *spd_data);
void 	__s5p_hdmi_init_hpd_onoff(bool on_off);
s5p_tv_hdmi_err __s5p_hdmi_audio_init(s5p_tv_audio_codec_type audio_codec, u32 sample_rate, u32 bits, u32 frame_size_code);
s32 hdmi_phy_config(phy_freq freq, s5p_hdmi_color_depth cd);

s5p_tv_hdmi_err __s5p_hdmi_video_init_display_mode(s5p_tv_disp_mode disp_mode, s5p_tv_o_mode out_mode);
void 	__s5p_hdmi_video_init_bluescreen(bool en, u8 cb, u8 y_g, u8 cr_r);
void 	__s5p_hdmi_video_init_color_range(u8 y_min, u8 y_max, u8 c_min, u8 c_max);
s5p_tv_hdmi_err __s5p_hdmi_video_init_csc(s5p_tv_hdmi_csc_type csc_type);
s5p_tv_hdmi_err __s5p_hdmi_video_init_avi_infoframe(s5p_hdmi_transmit trans_type, u8 check_sum, u8 *pavi_data);
s5p_tv_hdmi_err __s5p_hdmi_video_init_mpg_infoframe(s5p_hdmi_transmit trans_type, u8 check_sum, u8 *pmpg_data);
void 	__s5p_hdmi_video_init_tg_cmd(bool timing_correction_en, bool BT656_sync_en, s5p_tv_disp_mode disp_mode, bool tg_en);
bool 	__s5p_hdmi_start(s5p_hdmi_audio_type hdmi_audio_type, bool HDCP_en, struct i2c_client *ddc_port);
void 	__s5p_hdmi_stop(void);

// SDOUT
irqreturn_t s5p_tvenc_irq(int irq, void *dev_id);
//
s5p_tv_sd_err 	__s5p_sdout_init_video_scale_cfg(s5p_sd_level component_level, s5p_sd_vsync_ratio component_ratio, s5p_sd_level composite_level, s5p_sd_vsync_ratio composite_ratio);
s5p_tv_sd_err 	__s5p_sdout_init_sync_signal_pin(s5p_sd_sync_sig_pin pin);
s5p_tv_sd_err 	__s5p_sdout_init_vbi(bool wss_cvbs, s5p_sd_closed_caption_type caption_cvbs, bool wss_y_sideo, s5p_sd_closed_caption_type caption_y_sideo, bool cgmsa_rgb, bool wss_rgb, s5p_sd_closed_caption_type caption_rgb, bool cgmsa_y_ppr, bool wss_y_ppr, s5p_sd_closed_caption_type caption_y_ppr);
s5p_tv_sd_err 	__s5p_sdout_init_offset_gain(s5p_sd_channel_sel channel, u32 offset, u32 gain);
void 	__s5p_sdout_init_delay(u32 delay_y, u32 offset_video_start, u32 offset_video_end);
void 	__s5p_sdout_init_schlock(bool color_sucarrier_pha_adj);
s5p_tv_sd_err 	__s5p_sdout_init_dac_power_onoff(s5p_sd_channel_sel channel, bool dac_on);
s5p_tv_sd_err 	__s5p_sdout_init_macrovision(s5p_sd_macrovision_val macro, s5p_tv_disp_mode disp_mode);
void 	__s5p_sdout_init_color_compensaton_onoff(bool bright_hue_saturation_adj, bool y_ppr_color_compensation, bool rgb_color_compensation, bool y_c_color_compensation, bool y_cvbs_color_compensation);
void 	__s5p_sdout_init_brightness_hue_saturation(u32 gain_brightness, u32 offset_brightness, u32 gain0_cb_hue_saturation, u32 gain1_cb_hue_saturation, u32 gain0_cr_hue_saturation, u32 gain1_cr_hue_saturation, u32 offset_cb_hue_saturation, u32 offset_cr_hue_saturation);
void 	__s5p_sdout_init_rgb_color_compensation(u32 max_rgb_cube, u32 min_rgb_cube);
void 	__s5p_sdout_init_cvbs_color_compensation(u32 y_lower_mid, u32 y_bottom, u32 y_top, u32 y_upper_mid, u32 radius);
void 	__s5p_sdout_init_svideo_color_compensation(u32 y_top, u32 y_bottom, u32 y_c_cylinder);
void 	__s5p_sdout_init_component_porch(u32 back_525, u32 front_525, u32 back_625, u32 front_625);
s5p_tv_sd_err 	__s5p_sdout_init_vesa_rgb_sync(s5p_sd_vesa_rgb_sync_type sync_type, s5p_tv_active_polarity v_sync_active, s5p_tv_active_polarity h_sync_active);
void 	__s5p_sdout_init_oversampling_filter_coeff(u32 size, u32 *pcoeff0, u32 *pcoeff1, u32 *pcoeff2);
s5p_tv_sd_err 	__s5p_sdout_init_ch_xtalk_cancel_coef(s5p_sd_channel_sel channel, u32 coeff2, u32 coeff1);
void 	__s5p_sdout_init_closed_caption(u32 display_cc, u32 non_display_cc);
//static u32 	__s5p_sdout_init_wss_cgms_crc (u32 value);
s5p_tv_sd_err 	__s5p_sdout_init_wss525_data(s5p_sd_525_copy_permit copy_permit, s5p_sd_525_mv_psp mv_psp, s5p_sd_525_copy_info copy_info, bool analog_on, s5p_sd_525_aspect_ratio display_ratio);
s5p_tv_sd_err 	__s5p_sdout_init_wss625_data(bool surround_sound, bool copyright, bool copy_protection, bool text_subtitles, s5p_sd_625_subtitles open_subtitles, s5p_sd_625_camera_film camera_film, s5p_sd_625_color_encoding color_encoding, bool helper_signal, s5p_sd_625_aspect_ratio display_ratio);
s5p_tv_sd_err 	__s5p_sdout_init_cgmsa525_data(s5p_sd_525_copy_permit copy_permit, s5p_sd_525_mv_psp mv_psp, s5p_sd_525_copy_info copy_info, bool analog_on, s5p_sd_525_aspect_ratio display_ratio);
s5p_tv_sd_err 	__s5p_sdout_init_cgmsa625_data(bool surround_sound, bool copyright, bool copy_protection, bool text_subtitles, s5p_sd_625_subtitles open_subtitles, s5p_sd_625_camera_film camera_film, s5p_sd_625_color_encoding color_encoding, bool helper_signal, s5p_sd_625_aspect_ratio display_ratio);
//static s5p_tv_sd_err 	__s5p_sdout_init_antialias_filter_coeff_default (s5p_sd_level composite_level, s5p_sd_vsync_ratio composite_ratio, s5p_tv_o_mode out_mode);
//static s5p_tv_sd_err 	__s5p_sdout_init_oversampling_filter_coeff_default (s5p_tv_o_mode out_mode);
s5p_tv_sd_err 	__s5p_sdout_init_display_mode(s5p_tv_disp_mode disp_mode, s5p_tv_o_mode out_mode, s5p_sd_order order);
void 	__s5p_sdout_start(void);
void 	__s5p_sdout_stop(void);
void 	__s5p_sdout_sw_reset(bool active);
void 	__s5p_sdout_set_interrupt_enable(bool vsync_intr_en);
void 	__s5p_sdout_clear_interrupt_pending(void);
bool 	__s5p_sdout_get_interrupt_pending(void);

// VMIXER
s5p_tv_vmx_err 	__s5p_vm_set_win_blend(s5p_tv_vmx_layer layer, bool enable);
s5p_tv_vmx_err 	__s5p_vm_set_layer_alpha(s5p_tv_vmx_layer layer, u32 alpha);
s5p_tv_vmx_err 	__s5p_vm_set_layer_show(s5p_tv_vmx_layer layer, bool show);
s5p_tv_vmx_err 	__s5p_vm_set_layer_priority(s5p_tv_vmx_layer layer, u32 priority);
s5p_tv_vmx_err 	__s5p_vm_set_grp_base_address(s5p_tv_vmx_layer layer, u32 baseaddr);
s5p_tv_vmx_err 	__s5p_vm_set_grp_layer_position(s5p_tv_vmx_layer layer, u32 dst_offs_x, u32 dst_offs_y);
s5p_tv_vmx_err 	__s5p_vm_set_grp_layer_size(s5p_tv_vmx_layer layer, u32 span, u32 width, u32 height, u32 src_offs_x, u32 src_offs_y);
s5p_tv_vmx_err 	__s5p_vm_set_bg_color(s5p_tv_vmx_bg_color_num colornum, u32 color_y, u32 color_cb, u32 color_cr);
s5p_tv_vmx_err 	__s5p_vm_init_status_reg(s5p_vmx_burst_mode burst, s5p_endian_type endian);
s5p_tv_vmx_err 	__s5p_vm_init_display_mode(s5p_tv_disp_mode mode, s5p_tv_o_mode output_mode);
s5p_tv_vmx_err 	__s5p_vm_init_layer(s5p_tv_vmx_layer layer, bool show, bool winblending, u32 alpha, u32 priority, s5p_tv_vmx_color_fmt color, bool blankchange, bool pixelblending, bool premul, u32 blankcolor, u32 baseaddr, u32 span, u32 width, u32 height, u32 src_offs_x, u32 src_offs_y, u32 dst_offs_x, u32 dst_offs_y);
void 	__s5p_vm_init_bg_dither_enable(bool cr_dither_enable, bool cdither_enable, bool y_dither_enable);
s5p_tv_vmx_err 	__s5p_vm_init_bg_color(s5p_tv_vmx_bg_color_num color_num, u32 color_y, u32 color_cb, u32 color_cr);
s5p_tv_vmx_err 	__s5p_vm_init_csc_coef(s5p_yuv_fmt_component component, s5p_tv_coef_y_mode mode, u32 coeff0, u32 coeff1, u32 coeff2);
void 	__s5p_vm_init_csc_coef_default(s5p_tv_vmx_csc_type csc_type);
s5p_tv_vmx_err 	__s5p_vm_get_layer_info(s5p_tv_vmx_layer layer, bool *show, u32 *priority);
void 	__s5p_vm_start(void);
void 	__s5p_vm_stop(void);
s5p_tv_vmx_err 	__s5p_vm_set_underflow_interrupt_enable(s5p_tv_vmx_layer layer, bool en);
void __s5p_vm_clear_pend_all(void);
irqreturn_t __s5p_mixer_irq(int irq, void *dev_id);

#if 0
void __s5p_vm_save_reg(void);
void __s5p_vm_restore_reg(void);
#endif


// VPROCESSOR
void 	__s5p_vp_set_field_id(s5p_vp_field mode);
s5p_tv_vp_err 	__s5p_vp_set_top_field_address(u32 top_y_addr, u32 top_c_addr);
s5p_tv_vp_err 	__s5p_vp_set_bottom_field_address(u32 bottom_y_addr, u32 bottom_c_addr);
s5p_tv_vp_err 	__s5p_vp_set_img_size(u32 img_width, u32 img_height);
void 	__s5p_vp_set_src_position(u32 src_off_x, u32 src_x_fract_step, u32 src_off_y);
void 	__s5p_vp_set_dest_position(u32 dst_off_x, u32 dst_off_y);
void 	__s5p_vp_set_src_dest_size(u32 src_width, u32 src_height, u32 dst_width, u32 dst_height, bool ipc_2d);
s5p_tv_vp_err 	__s5p_vp_set_poly_filter_coef(s5p_vp_poly_coeff poly_coeff, signed char ch0, signed char ch1, signed char ch2, signed char ch3);
void 	__s5p_vp_set_poly_filter_coef_default(u32 h_ratio, u32 v_ratio);
void 	__s5p_vp_set_src_dest_size_with_default_poly_filter_coef(u32 src_width, u32 src_height, u32 dst_width, u32 dst_height, bool ipc_2d);
s5p_tv_vp_err 	__s5p_vp_set_brightness_contrast_control(s5p_vp_line_eq eq_num, u32 intc, u32 slope);
void 	__s5p_vp_set_brightness(bool brightness);
void 	__s5p_vp_set_contrast(u8 contrast);
s5p_tv_vp_err 	__s5p_vp_update(void);
s5p_vp_field 	__s5p_vp_get_field_id(void);
bool 	__s5p_vp_get_update_status(void);
void 	__s5p_vp_init_field_id(s5p_vp_field mode);
void 	__s5p_vp_init_op_mode(bool line_skip, s5p_vp_mem_mode mem_mode, s5p_vp_chroma_expansion chroma_exp, s5p_vp_filed_id_toggle toggle_id);
void 	__s5p_vp_init_pixel_rate_control(s5p_vp_pxl_rate rate);
s5p_tv_vp_err 	__s5p_vp_init_layer(u32 top_y_addr, u32 top_c_addr, u32 bottom_y_addr, u32 bottom_c_addr, s5p_endian_type src_img_endian, u32 img_width, u32 img_height, u32 src_off_x, u32 src_x_fract_step, u32 src_off_y, u32 src_width, u32 src_height, u32 dst_off_x, u32 dst_off_y, u32 dst_width, u32 dst_height, bool ipc_2d);
s5p_tv_vp_err 	__s5p_vp_init_layer_def_poly_filter_coef(u32 top_y_addr, u32 top_c_addr, u32 bottom_y_addr, u32 bottom_c_addr, s5p_endian_type src_img_endian, u32 img_width, u32 img_height, u32 src_off_x, u32 src_x_fract_step, u32 src_off_y, u32 src_width, u32 src_height, u32 dst_off_x, u32 dst_off_y, u32 dst_width, u32 dst_height, bool ipc_2d);
s5p_tv_vp_err 	__s5p_vp_init_poly_filter_coef(s5p_vp_poly_coeff poly_coeff, signed char ch0, signed char ch1, signed char ch2, signed char ch3);
void 	__s5p_vp_init_bypass_post_process(bool bypass);
s5p_tv_vp_err 	__s5p_vp_init_csc_coef(s5p_vp_csc_coeff csc_coeff, u32 coeff);
void 	__s5p_vp_init_saturation(u32 sat);
void 	__s5p_vp_init_sharpness(u32 th_h_noise, s5p_vp_sharpness_control sharpness);
s5p_tv_vp_err 	__s5p_vp_init_brightness_contrast_control(s5p_vp_line_eq eq_num, u32 intc, u32 slope);
void 	__s5p_vp_init_brightness(bool brightness);
void 	__s5p_vp_init_contrast(u8 contrast);
void 	__s5p_vp_init_brightness_offset(u32 offset);
void 	__s5p_vp_init_csc_control(bool suy_offset_en, bool csc_en);
s5p_tv_vp_err 	__s5p_vp_init_csc_coef_default(s5p_vp_csc_type csc_type);
s5p_tv_vp_err 	__s5p_vp_start(void);
s5p_tv_vp_err 	__s5p_vp_stop(void);
void 	__s5p_vp_sw_reset(void);




// TV_CLOCK
#ifdef CONFIG_CPU_S5PC100
void    __s5p_tv_clk_init_hpll(u32 lock_time, u32 mdiv, u32 pdiv, u32 sdiv);
#endif

#ifdef CONFIG_CPU_S5PC110
void    __s5p_tv_clk_init_hpll(bool vsel,u32 lock_time, u32 mdiv, u32 pdiv, u32 sdiv);
#endif
//

void __s5p_tv_clk_hpll_onoff(bool en);





s5p_tv_clk_err 	__s5p_tv_clk_init_href(s5p_tv_clk_hpll_ref hpll_ref);
s5p_tv_clk_err 	__s5p_tv_clk_init_mout_hpll(s5p_tv_clk_mout_hpll mout_hpll);
s5p_tv_clk_err 	__s5p_tv_clk_init_video_mixer(s5p_tv_clk_vmiexr_srcclk src_clk);
void 	__s5p_tv_clk_init_hdmi_ratio(u32 clk_div);
void 	__s5p_tv_clk_set_vp_clk_onoff(bool clk_on);
void 	__s5p_tv_clk_set_vmixer_hclk_onoff(bool clk_on);
void 	__s5p_tv_clk_set_vmixer_sclk_onoff(bool clk_on);
void 	__s5p_tv_clk_set_sdout_hclk_onoff(bool clk_on);
void 	__s5p_tv_clk_set_sdout_sclk_onoff(bool clk_on);
void 	__s5p_tv_clk_set_hdmi_hclk_onoff(bool clk_on);
void 	__s5p_tv_clk_set_hdmi_sclk_onoff(bool clk_on);
void 	__s5p_tv_clk_set_hdmi_i2c_clk_onoff(bool clk_on);
void 	__s5p_tv_clk_start(bool vp_hclk_on, bool sdout_hclk_on, bool hdmi_hclk_on);
void 	__s5p_tv_clk_stop(void);

// TV_POWER
void 	__s5p_tv_power_init_mtc_stable_counter(u32 value);
void 	__s5p_tv_powerinitialize_dac_onoff(bool on);
void 	__s5p_tv_powerset_dac_onoff(bool on);
bool 	__s5p_tv_power_get_power_status(void);
bool 	__s5p_tv_power_get_dac_power_status(void);
void 	__s5p_tv_poweron(void);
void 	__s5p_tv_poweroff(void);

extern s5p_tv_status s5ptv_status;
extern s5p_tv_vo s5ptv_overlay[2];

