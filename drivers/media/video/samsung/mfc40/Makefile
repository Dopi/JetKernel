obj-$(CONFIG_VIDEO_MFC40) += h263_dec_fw.o h264_dec_fw.o h264_enc_fw.o mp2_dec_fw.o mp4_dec_fw.o mp4_enc_fw.o s3c-mfc.o s3c_mfc_buffer_manager.o s3c_mfc_common.o s3c_mfc_intr.o s3c_mfc_memory.o s3c_mfc_opr.o vc1_dec_fw.o command_control_fw.o

EXTRA_CFLAGS += -DLINUX
EXTRA_CFLAGS += -DDIVX_ENABLE

ifeq ($(CONFIG_VIDEO_MFC40_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif
