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

# CPU support

obj-$(CONFIG_CPU_S3C64XX_INIT)	+= s3c64xx-init.o
obj-$(CONFIG_CPU_S3C64XX_CLOCK)	+= s3c64xx-clock.o
obj-$(CONFIG_CPU_FREQ)		+= s3c64xx-dvfs.o changefreq.o
obj-$(CONFIG_PM)                += pm.o
obj-$(CONFIG_PM)                += sleep.o

# Device setup

obj-$(CONFIG_S3C64XX_SETUP_I2C0) += setup-i2c0.o
obj-$(CONFIG_S3C64XX_SETUP_I2C1) += setup-i2c1.o
obj-$(CONFIG_S3C64XX_ADCTS) += s3c64xx-adcts.o

#
# Makefile for PWM drivers.
#
obj-$(CONFIG_S3C6410_PWM)       += pwm-s3c6410.o