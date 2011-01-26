#
#   Makefile S3C Camera driver
#
#   Copyright(c) 2004-2006, Samsung Electronics, Co., Ltd.
#

ifeq ($(CONFIG_MACH_INFOBOWLQ),y)
EXTRA_CFLAGS 	+= -DCONFIG_VIDEO_SAMSUNG_S5K4CA -DCONFIG_FLASH_AAT1271A 
endif

SENSOR_DIR		= samsung

#
# S3C Camera Interface
#

obj-y			+= s3c_camera.o

s3c_camera-y	+= s3c_camif.o s3c_camera_driver.o


#
# Camera Flash
#

ifeq ($(CONFIG_MACH_INFOBOWLQ),y)
s3c_camera-y += $(SENSOR_DIR)/flash/aat1271a.o
endif
ifeq ($(CONFIG_MACH_CYGNUS),y)
#s3c_camera-y += $(SENSOR_DIR)/flash/aat1271a.o
endif

obj-$(CONFIG_VIDEO_SAMSUNG)	+= samsung/
