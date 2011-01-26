# arch/arm/plat-s3c6410/Makefile
#
# Copyright 2008 Openmoko, Inc.
# Copyright 2008 Simtec Electronics
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:=
obj-				:=

# Core support for S3C6410 system

obj-$(CONFIG_CPU_S3C6410)	+= cpu.o
obj-$(CONFIG_S3C_DMA_PL080)	+= dma.o
obj-$(CONFIG_CPU_S3C6410)	+= irq.o

# Helper and device support

obj-$(CONFIG_S3C6410_SETUP_SDHCI)	+= setup-sdhci.o
obj-$(CONFIG_PM)    += pm.o

# machine support

obj-$(CONFIG_MACH_INFOBOWLQ)	+= mach-infobowlq.o board-rfkill.o
obj-$(CONFIG_SEC_HEADSET)	+= sec_headset.o
