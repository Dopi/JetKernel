#
# Makefile for the s3c framebuffer driver
#

ifeq ($(CONFIG_FB_S3C),y)
obj-$(CONFIG_FB_S3C)		+= s3cfb.o
obj-$(CONFIG_FB_S3C)		+= s3cfb_spi.o
obj-$(CONFIG_PLAT_S3C64XX)	+= s3cfb_fimd4x.o
obj-$(CONFIG_FB_S3C_LTE480WV)	+= s3cfb_lte480wv.o
obj-$(CONFIG_FB_S3C_LTV350QV)	+= s3cfb_ltv350qv.o
obj-$(CONFIG_FB_S3C_LTS222QV)	+= s3cfb_lts222qv.o
obj-$(CONFIG_FB_S3C_S6D05A0)	+= s3cfb_s6d05a0.o
obj-$(CONFIG_FB_S3C_AMS320FS01)	+= s3cfb_ams320fs01.o
endif

#ifeq ($(CONFIG_FB_S3C_V2),y)
obj-y				+= s3cfb2.o
obj-$(CONFIG_PLAT_S3C64XX)	+= s3cfb2_fimd4x.o

obj-$(CONFIG_FB_S3C_TL2796)     += s3cfb2_tl2796.o
#endif

ifeq ($(CONFIG_FB_S3C_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

