#################################################
# Makefile for TVOut for S5PC100
# 2009 (C) Samsung Electronics
# Author : sangpil moon <sangpil.moon@samsung.com>
#################################################

# where the kernel sources are located
#KERNEL_DIR := /home/spmoon/spmoon/09_linuxMobile/17_C100_TVSUB/C100_KERNEL/samsung-ap-2.6

#VPLL_USES_HDMI27M = 1
#TVENC_USES_HDMIPHY = 1
obj-$(CONFIG_VIDEO_TV20)		:= 	s5p_tvout.o
#obj-$(CONFIG_S5PC100_TVOUT)		:=  	s5p_tvout.o
ifeq ($(CONFIG_CPU_S5PC100),y)
s5p_tvout-y	+=	s5pc100/hdcp_s5pc100.o \
			s5pc100/hdmi_s5pc100.o  \
			s5pc100/sdout_s5pc100.o  \
			s5pc100/tv_power_s5pc100.o  \
			s5pc100/vmixer_s5pc100.o  \
			s5pc100/vprocessor_s5pc100.o \
			s5pc100/tv_clock_s5pc100.o  
endif

ifeq ($(CONFIG_CPU_S5PC110),y)
s5p_tvout-y	+=	s5pc110/hdcp_s5pc110.o \
			s5pc110/hdmi_s5pc110.o  \
			s5pc110/sdout_s5pc110.o  \
			s5pc110/tv_power_s5pc110.o  \
			s5pc110/vmixer_s5pc110.o  \
			s5pc110/vprocessor_s5pc110.o \
			s5pc110/tv_clock_s5pc110.o  
endif

s5p_tvout-y	+=	s5p_stda_tvout_if.o \
			s5p_stda_grp.o \
			s5p_stda_hdmi.o \
			s5p_stda_video_layer.o \
			s5p_tv_v4l2.o \
			s5p_tv_base.o 

ifeq ($(CONFIG_VIDEO_TV20_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif			
