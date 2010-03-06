
ifeq ($(CONFIG_PM_DEBUG),y)
EXTRA_CFLAGS	+=	-DDEBUG
endif

obj-y				:= main.o
obj-$(CONFIG_PM_SLEEP)		+= process.o console.o
obj-$(CONFIG_WAKELOCK)		+= wakelock.o
obj-$(CONFIG_USER_WAKELOCK)	+= userwakelock.o
obj-$(CONFIG_EARLYSUSPEND)	+= earlysuspend.o
obj-$(CONFIG_CONSOLE_EARLYSUSPEND)	+= consoleearlysuspend.o
obj-$(CONFIG_FB_EARLYSUSPEND)	+= fbearlysuspend.o
obj-$(CONFIG_HIBERNATION)	+= swsusp.o disk.o snapshot.o swap.o user.o

obj-$(CONFIG_MAGIC_SYSRQ)	+= poweroff.o
