ifeq ($(CONFIG_MARVELL_8686_PROC_FS),y)
	sd8686-objs += wlan_proc.o
endif

ifeq ($(CONFIG_MARVELL_8686_DEBUG),y)
	sd8686-objs += wlan_debug.o
endif

sd8686-objs += wlan_main.o wlan_fw.o \
		wlan_wext.o wlan_rx.o wlan_tx.o \
		wlan_cmd.o wlan_cmdresp.o wlan_scan.o \
		wlan_join.o wlan_wmm.o wlan_11d.o wlan_fops.o \
		if_sdio.o
obj-$(CONFIG_MARVELL_8686_SDIO) += sd8686.o

