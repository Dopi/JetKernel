#ifndef _SEC_LOG_H_
#define _SEC_LOG_H_

struct struct_plat_log_mark {

	u32 special_mark_1;

	u32 special_mark_2;

	u32 special_mark_3;

	u32 special_mark_4;

	void *p_main;

	void *p_radio;

	void *p_events;
};

struct struct_kernel_log_mark {

	u32 special_mark_1;

	u32 special_mark_2;

	u32 special_mark_3;

	u32 special_mark_4;

	void *p__log_buf;

};

struct struct_frame_buf_mark {

	u32 special_mark_1;

	u32 special_mark_2;

	u32 special_mark_3;

	u32 special_mark_4;

	void *p_fb;
	u32 resX;
	u32 resY;
	u32 bpp;
	u32 frames;
};

struct struct_marks_ver_mark {

	u32 special_mark_1;

	u32 special_mark_2;

	u32 special_mark_3;

	u32 special_mark_4;

	u32 log_mark_version;

	u32 framebuffer_mark_version;
};


#endif
