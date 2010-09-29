
/*
 * Parameter Infomation
 */

#ifndef ASM_MACH_PARAM_H
#define ASM_MACH_PARAM_H

#define PARAM_MAGIC			0x72726624
#define PARAM_VERSION		0x11	/* Rev 1.1 */
#define PARAM_STRING_SIZE	1024	/* 1024 Characters */

#define MAX_PARAM			20
#define MAX_STRING_PARAM	5

/* Default Parameter Values */

#define SERIAL_SPEED		7		/* Baudrate */
#define LCD_LEVEL			0x07F	/* Backlight Level */
#define BOOT_DELAY			0		/* Boot Wait Time */
#define LOAD_RAMDISK		0		/* Enable Ramdisk Loading */
#define SWITCH_SEL			1		/* Switch Setting (UART[1], USB[0]) */
#define PHONE_DEBUG_ON		0		/* Enable Phone Debug Mode */
#define LCD_DIM_LEVEL		0x01D	/* Backlight Dimming Level */
#define MELODY_MODE			0		/* Melody Mode */
#define REBOOT_MODE			0		/* Reboot Mode */
#define NATION_SEL			0		/* Language Configuration */
#define SET_DEFAULT_PARAM	0		/* Set Param to Default */
#define VERSION_LINE		"INSTINCTQXXIE00"	/* Set Image Info */
#define COMMAND_LINE		"console=ttySAC2,115200"
#define	BOOT_VERSION		" version=Sbl(1.0.0) "

typedef enum {
	__SERIAL_SPEED,
	__LOAD_RAMDISK,
	__BOOT_DELAY,
	__LCD_LEVEL,
	__SWITCH_SEL,
	__PHONE_DEBUG_ON,
	__LCD_DIM_LEVEL,
	__MELODY_MODE,
	__REBOOT_MODE,
	__NATION_SEL,
	__SET_DEFAULT_PARAM,
	__PARAM_INT_11,
	__PARAM_INT_12,
	__PARAM_INT_13,
	__PARAM_INT_14,
	__VERSION,
	__CMDLINE,
	__PARAM_STR_2,
	__PARAM_STR_3,
	__PARAM_STR_4
} param_idx;

typedef struct _param_int_t {
	param_idx ident;
	int  value;
} param_int_t;

typedef struct _param_str_t {
	param_idx ident;
	char value[PARAM_STRING_SIZE];
} param_str_t;

typedef struct {
	int param_magic;
	int param_version;
	param_int_t param_list[MAX_PARAM - MAX_STRING_PARAM];
	param_str_t param_str_list[MAX_STRING_PARAM];
} status_t;

/* REBOOT_MODE */
#define REBOOT_MODE_NONE		0
#define REBOOT_MODE_DOWNLOAD		1
#define REBOOT_MODE_CHARGING		3
#define REBOOT_MODE_RECOVERY		4
#define REBOOT_MODE_ARM11_FOTA		5
#define REBOOT_MODE_ARM9_FOTA		6

extern void (*sec_set_param_value)(int idx, void *value);
extern void (*sec_get_param_value)(int idx, void *value);

#define USB_SEL_MASK	(1 << 0)
#define UART_SEL_MASK	(1 << 1)

#endif	/* ASM_MACH_PARAM_H */
