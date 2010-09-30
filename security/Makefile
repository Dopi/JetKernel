#
# Makefile for the kernel security code
#

obj-$(CONFIG_KEYS)			+= keys/
subdir-$(CONFIG_SECURITY_SELINUX)	+= selinux
subdir-$(CONFIG_SECURITY_SMACK)		+= smack
subdir-$(CONFIG_SECURITY_TOMOYO)        += tomoyo

# always enable default capabilities
obj-y		+= commoncap.o min_addr.o

# Object file lists
obj-$(CONFIG_SECURITY)			+= security.o capability.o
obj-$(CONFIG_SECURITYFS)		+= inode.o
# Must precede capability.o in order to stack properly.
obj-$(CONFIG_SECURITY_SELINUX)		+= selinux/built-in.o
obj-$(CONFIG_SECURITY_SMACK)		+= smack/built-in.o
ifeq ($(CONFIG_AUDIT),y)
obj-$(CONFIG_SECURITY_SMACK)		+= lsm_audit.o
endif
obj-$(CONFIG_SECURITY_TOMOYO)		+= tomoyo/built-in.o
obj-$(CONFIG_SECURITY_ROOTPLUG)		+= root_plug.o
obj-$(CONFIG_CGROUP_DEVICE)		+= device_cgroup.o

# Object integrity file lists
subdir-$(CONFIG_IMA)			+= integrity/ima
obj-$(CONFIG_IMA)			+= integrity/ima/built-in.o
