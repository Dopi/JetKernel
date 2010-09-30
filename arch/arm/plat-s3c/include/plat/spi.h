/* linux/arch/arm/plat-s3c/include/plat/spi.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __PLAT_SPI_H
#define __PLAT_SPI_H __FILE__

#define CS_ACT_LOW	0
#define CS_ACT_HIGH	1

#define CS_FLOAT	-1
#define CS_LOW		0
#define CS_HIGH		1
#define CS_TOGGLE	2

struct s3c_spi_pdata {
	int cs_level;		/* Current level of the CS line */
	int cs_pin;	/* GPIO Pin number used as the ChipSelect for this Slave */
	int cs_mode;
	void (*cs_config)(int pin, int mode, int pull); /* Configure the GPIO in appropriate mode(output) initially */
	void (*cs_set)(int pin, int lvl); /* CS line control */
	void (*cs_suspend)(int pin, pm_message_t pm); /* To save power */
	void (*cs_resume)(int pin);
};

struct s3c_spi_mstr_info {
	int num_slaves;
	struct clk *clk;
	struct clk *prnt_clk; /* PCLK, USBCLK or Epll_CLK */
	struct platform_device *pdev;
	struct s3c_spi_pdata *spd; /* We index in this array to get func pointers */
	int (*spiclck_get)(struct s3c_spi_mstr_info *);
	void (*spiclck_put)(struct s3c_spi_mstr_info *);
	int (*spiclck_en)(struct s3c_spi_mstr_info *);
	void (*spiclck_dis)(struct s3c_spi_mstr_info *);
	u32 (*spiclck_getrate)(struct s3c_spi_mstr_info *);
	int (*spiclck_setrate)(struct s3c_spi_mstr_info *, u32);
};

#define BUSNUM(b)		(b)

extern void s3cspi_set_slaves(unsigned, int, struct s3c_spi_pdata const *);

#endif /* __PLAT_SPI_H */
