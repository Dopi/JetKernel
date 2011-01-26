EXTRA_CFLAGS += -DBCMDBG -DBCMDBG_MEM -Dlinux -DLINUX -DBDC -DTOE -DBCMDRIVER -DBCMDONGLEHOST -DDHDTHREAD -DBCMWPA2 -DUSE_STOCK_MMC_DRIVER -DDHD_GPL -DDHD_SCHED -DBCMSDIO -DDHD_GPL -DBCMLXSDMMC -DBCMPLATFORM_BUS -DDHD_BCMEVENTS -DSHOW_EVENTS -DDHD_DEBUG -DSRCBASE=\"$(src)/src\" -DANDROID_SPECIFIC -DMAX_HDR_LEN=64 -DDHD_FIRSTREAD=64 -DSDIO_ISR_THREAD -DENABLE_DEEP_SLEEP -DWRITE_MACADDR

#################################################################### OPTIONAL FEATURES ###################################################################################
#TO ENALBLE OPTIONAL FEATURES UNCOMMENT THE CORRESPONDING FLAGS

#   HOST WAKEUP
EXTRA_CFLAGS += -DCONFIG_BRCM_GPIO_INTR -DBCM_HOSTWAKE -DBCMHOSTWAKE_IRQ

#   PACKET FILTER AND ARP OFFLOADING.            ****DEPENDENCY ==> HOSTWAKEUP.
EXTRA_CFLAGS += -DBCM_PKTFILTER -DBCM_ARPO

#   WAPI
EXTRA_CFLAGS += -DBCMWAPI_WPI

#   WAPI-CERT [To Disable Power save option.]    ****DEPENDENCY ==> WAPI
#EXTRA_CFLAGS += -DBCMDISABLE_PM

#   SOFTAP
EXTRA_CFLAGS += -DSOFTAP

#   STATIC MEMORY ALLOCATION FEATURE
EXTRA_CFLAGS += -DDHD_USE_STATIC_BUF

#   For Samsung Infobowlq only.
EXTRA_CFLAGS += -DCUSTOMER_HW_SAMSUNG

###########################################################################################################################################################################
ifeq ($(CONFIG_BCM_EMBED_FW),y)
	EXTRA_CFLAGS		+= -I$(src)/src/dongle/rte/wl/builds/4325b0/sdio-g-cdc-reclaim-wme/
	EXTRA_CFLAGS		+= -DBCMEMBEDIMAGE
	EXTRA_CFLAGS		+= -DIMAGE_NAME="4325b0/sdio-g-cdc-reclaim-wme"
endif

EXTRA_CFLAGS += -I$(src)/src/include/
EXTRA_CFLAGS += -I$(src)/src/dhd/sys/
EXTRA_CFLAGS += -I$(src)/src/dongle/
EXTRA_CFLAGS += -I$(src)/src/bcmsdio/sys/
EXTRA_CFLAGS += -I$(src)/src/wl/sys/
EXTRA_CFLAGS += -I$(src)/src/shared/

KBUILD_CFLAGS += -I$(LINUXDIR)/include -I$(shell pwd)


#obj-$(CONFIG_BROADCOM_WIFI)-m	+= bcm4325.o
obj-m	+= dhd.o

dhd-y := src/dhd/sys/dhd_linux.o \
		src/shared/bcmutils.o \
		src/dhd/sys/dhd_common.o \
		src/shared/siutils.o \
		src/shared/sbutils.o \
		src/shared/aiutils.o \
		src/shared/hndpmu.o \
		src/wl/sys/wl_iw.o \
		src/shared/bcmwifi.o \
		src/dhd/sys/dhd_cdc.o \
		src/dhd/sys/dhd_linux_sched.o\
		src/dhd/sys/dhd_sdio.o \
		src/dhd/sys/dhd_custom_gpio.o \
		src/bcmsdio/sys/bcmsdh_sdmmc.o \
		src/bcmsdio/sys/bcmsdh.o \
		src/bcmsdio/sys/bcmsdh_linux.o \
		src/bcmsdio/sys/bcmsdh_sdmmc_linux.o \
		src/bcmsdio/sys/wlgpio.o \
		src/shared/linux_osl.o
all:
	@echo "$(MAKE) --no-print-directory -C $(KDIR) SUBDIRS=$(CURDIR) modules"
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules

clean: 
	rm -rf *.o *.ko *.mod.c *~ .*.cmd Module.symvers modules.order .tmp_versions	\
	 	src/dhd/sys/dhd_linux.o \
		src/shared/bcmutils.o \
		src/dhd/sys/dhd_common.o \
		src/shared/siutils.o \
		src/shared/sbutils.o \
		src/shared/aiutils.o \
		src/shared/hndpmu.o \
		src/wl/sys/wl_iw.o \
		src/shared/bcmwifi.o \
		src/dhd/sys/dhd_cdc.o \
		src/dhd/sys/dhd_linux_sched.o\
		src/dhd/sys/dhd_sdio.o \
		src/dhd/sys/dhd_custom_gpio.o \
		src/bcmsdio/sys/bcmsdh_sdmmc.o \
		src/bcmsdio/sys/bcmsdh.o \
		src/bcmsdio/sys/bcmsdh_linux.o \
		src/bcmsdio/sys/bcmsdh_sdmmc_linux.o \
		src/bcmsdio/sys/wlgpio.o \
		src/shared/linux_osl.o

install:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules_install
