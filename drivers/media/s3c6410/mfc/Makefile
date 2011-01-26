#
#   Makefile for G3D
#
#   Copyright(c) 2004-2006, Samsung Electronics, Co., Ltd.
#

EXTRA_CFLAGS 	+= -DLINUX
EXTRA_CFLAGS 	+= -DDIVX_ENABLE

s3c_mfc-y 		:= Prism_S_V13F.o BitProcBuf.o DataBuf.o FramBufMgr.o \
				   LogMsg.o MFC_HW_Init.o MFC_Inst_Pool.o MFC_Instance.o MfcMemory.o MfcMutex.o MfcSfr.o \
				   s3c-mfc.o MfcIntrNotification.o MfcSetConfig.o

obj-$(CONFIG_VIDEO_MFC)	:= s3c_mfc.o 
