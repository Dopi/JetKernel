obj-$(CONFIG_VIDEO_FIMC)	+= fimc_dev.o fimc_v4l2.o fimc_capture.o fimc_output.o fimc_overlay.o
obj-$(CONFIG_VIDEO_FIMC_MIPI)	+= csis.o
obj-$(CONFIG_VIDEO_IPC)		+= ipc.o
obj-$(CONFIG_CPU_S5PC100)	+= fimc40_regs.o
obj-$(CONFIG_CPU_S5PC110)	+= fimc43_regs.o

ifeq ($(CONFIG_CPU_S5PC110),y)
EXTRA_CFLAGS += -DCONFIG_MIPI_CSI_ADV_FEATURE
endif

EXTRA_CFLAGS += -Idrivers/media/video

ifeq ($(CONFIG_VIDEO_FIMC_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

