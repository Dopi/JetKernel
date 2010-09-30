# arch/arm/plat-s3c64xx/Makefile
#
# Copyright 2008 Openmoko, Inc.
# Copyright 2008 Simtec Electronics
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:= dummy.o
obj-				:=

# Core files

obj-y				+= dev-uart.o devs.o
obj-y				+= cpu.o
obj-y				+= irq.o
obj-y				+= irq-eint.o
obj-y				+= clock.o
obj-y				+= gpiolib.o
obj-y				+= bootmem.o

# CPU support

obj-$(CONFIG_CPU_S3C6400_INIT)	+= s3c6400-init.o
#obj-$(CONFIG_CPU_S3C6400_CLOCK)	+= s3c6400-clock.o
obj-$(CONFIG_CPU_S3C6400_CLOCK)	+= s3c64xx-clock.o
#obj-$(CONFIG_CPU_FREQ_S3C64XX)  += cpufreq.o

# PM support

obj-$(CONFIG_PM)		+= pm.o
obj-$(CONFIG_PM)		+= sleep.o
obj-$(CONFIG_PM)		+= irq-pm.o

# DMA support

obj-$(CONFIG_S3C64XX_DMA)	+= dma.o

# Device setup

obj-$(CONFIG_S3C64XX_SETUP_I2C0) += setup-i2c0.o
obj-$(CONFIG_S3C64XX_SETUP_I2C1) += setup-i2c1.o
obj-$(CONFIG_S3C64XX_SETUP_FB_24BPP) += setup-fb-24bpp.o
obj-$(CONFIG_CPU_FREQ_S3C64XX)		+= s3c64xx-dvfs.o changefreq.o
#obj-$(CONFIG_S3C64XX_SETUP_SDHCI_GPIO) += setup-sdhci-gpio.o ADDED ALMAR



obj-$(CONFIG_S3C64XX_ADC) 	 			+= adc.o

obj-$(CONFIG_SPI_S3C6410)   			+= dev-spi.o

# Device setup - FIMC
obj-$(CONFIG_S3C64XX_DEV_FIMC0)   += dev-fimc0.o
obj-$(CONFIG_S3C64XX_DEV_FIMC1)   += dev-fimc1.o
obj-$(CONFIG_S3C64XX_SETUP_FIMC0) += setup-fimc0.o
obj-$(CONFIG_S3C64XX_SETUP_FIMC1) += setup-fimc1.o

obj-$(CONFIG_S3C64XX_DEV_RP)      += dev-rp.o
obj-$(CONFIG_HAVE_PWM)		  			+= pwm.o
obj-$(CONFIG_S3C6410_PWM)	  			+= pwm-s3c6410.o
