# Machine Support
snd-soc-smdk6410-s5m8751-objs := smdk6410_s5m8751.o
snd-soc-smdk6410-wm8580-objs  := smdk6410_wm8580.o
snd-soc-smdk6410-wm9713-objs  := smdk6410_wm9713.o

snd-soc-smdk6440-s5m8751-objs := smdk6440_s5m8751.o
snd-soc-smdk6440-wm8580-objs  := smdk6440_wm8580.o

snd-soc-smdkc100-wm8580-objs  := smdkc100_wm8580.o
snd-soc-smdkc100-s5m8751-objs := smdkc100_s5m8751.o
snd-soc-smdkc100-wm9713-objs  := smdkc100_wm9713.o

snd-soc-s3c6410-i2s-objs := s3c6410-i2s.o

obj-$(CONFIG_SND_S3C64XX_SOC)	 	+= s3c-pcm.o
obj-$(CONFIG_SND_S3C64XX_SOC_SOL)	+= s3c-pcm-sol.o
obj-$(CONFIG_SND_S5P64XX_SOC) 		+= s3c-pcm.o
obj-$(CONFIG_SND_S5PC1XX_SOC) 		+= s3c-pcm.o

obj-$(CONFIG_SND_S3C_I2S_V32)  += s3c6410-i2s.o
#obj-$(CONFIG_SND_S3C_I2S_V32)  += s3c-i2s.o
obj-$(CONFIG_SND_S3C_I2S_V40)  += s3c-i2s.o
obj-$(CONFIG_SND_S3C_I2S_V50)  += s3c-i2s.o
obj-$(CONFIG_SND_S3C_AC97)     += s3c-ac97.o

snd-soc-ak4671-objs := s3c6410-ak4671.o

# S3C6410 has AC97 and I2S(v-3.2 and 4.0) controller.
# SMDK6410 has WM8580 and WM9713 onboard and S5M8751 as a module.
obj-$(CONFIG_SND_SMDK6410_S5M8751) += snd-soc-smdk6410-s5m8751.o
obj-$(CONFIG_SND_SMDK6410_WM8580)  += snd-soc-smdk6410-wm8580.o
obj-$(CONFIG_SND_SMDK6410_WM9713)  += snd-soc-smdk6410-wm9713.o

# S5P6440 has only I2S(v-4.0) controller.
# SMDK6410 has WM8580 onboard and S5M8751 as a module.
obj-$(CONFIG_SND_SMDK6440_S5M8751) += snd-soc-smdk6440-s5m8751.o
obj-$(CONFIG_SND_SMDK6440_WM8580)  += snd-soc-smdk6440-wm8580.o

# S5PC100 has AC97 and I2S(v-3.2 and 5.0) controller.
# SMDKC100 has WM8580 and WM9713 onboard and S5M8751 as a module.
obj-$(CONFIG_SND_SMDKC100_S5M8751) += snd-soc-smdkc100-s5m8751.o
obj-$(CONFIG_SND_SMDKC100_WM8580)  += snd-soc-smdkc100-wm8580.o
obj-$(CONFIG_SND_SMDKC100_WM9713)  += snd-soc-smdkc100-wm9713.o

# S3C6410 has AK4671 and I2S(v-3.2 and 4.0) controller.
obj-$(CONFIG_SND_S3C64XX_SOC_AK4671) += snd-soc-ak4671.o
