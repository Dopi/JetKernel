/***************************************************************************/

/*
 *	linux/arch/m68knommu/platform/5307/config.c
 *
 *	Copyright (C) 1999-2002, Greg Ungerer (gerg@snapgear.com)
 *	Copyright (C) 2000, Lineo (www.lineo.com)
 */

/***************************************************************************/

#include <linux/kernel.h>
#include <linux/param.h>
#include <linux/init.h>
#include <linux/io.h>
#include <asm/machdep.h>
#include <asm/coldfire.h>
#include <asm/mcfsim.h>
#include <asm/mcfuart.h>
#include <asm/mcfwdebug.h>

/***************************************************************************/

extern unsigned int mcf_timervector;
extern unsigned int mcf_profilevector;
extern unsigned int mcf_timerlevel;

/***************************************************************************/

/*
 *	Some platforms need software versions of the GPIO data registers.
 */
unsigned short ppdata;
unsigned char ledbank = 0xff;

/***************************************************************************/

static struct mcf_platform_uart m5307_uart_platform[] = {
	{
		.mapbase	= MCF_MBAR + MCFUART_BASE1,
		.irq		= 73,
	},
	{
		.mapbase 	= MCF_MBAR + MCFUART_BASE2,
		.irq		= 74,
	},
	{ },
};

static struct platform_device m5307_uart = {
	.name			= "mcfuart",
	.id			= 0,
	.dev.platform_data	= m5307_uart_platform,
};

static struct platform_device *m5307_devices[] __initdata = {
	&m5307_uart,
};

/***************************************************************************/

static void __init m5307_uart_init_line(int line, int irq)
{
	if (line == 0) {
		writeb(MCFSIM_ICR_LEVEL6 | MCFSIM_ICR_PRI1, MCF_MBAR + MCFSIM_UART1ICR);
		writeb(irq, MCF_MBAR + MCFUART_BASE1 + MCFUART_UIVR);
		mcf_setimr(mcf_getimr() & ~MCFSIM_IMR_UART1);
	} else if (line == 1) {
		writeb(MCFSIM_ICR_LEVEL6 | MCFSIM_ICR_PRI2, MCF_MBAR + MCFSIM_UART2ICR);
		writeb(irq, MCF_MBAR + MCFUART_BASE2 + MCFUART_UIVR);
		mcf_setimr(mcf_getimr() & ~MCFSIM_IMR_UART2);
	}
}

static void __init m5307_uarts_init(void)
{
	const int nrlines = ARRAY_SIZE(m5307_uart_platform);
	int line;

	for (line = 0; (line < nrlines); line++)
		m5307_uart_init_line(line, m5307_uart_platform[line].irq);
}

/***************************************************************************/

void mcf_autovector(unsigned int vec)
{
	volatile unsigned char  *mbar;

	if ((vec >= 25) && (vec <= 31)) {
		mbar = (volatile unsigned char *) MCF_MBAR;
		vec = 0x1 << (vec - 24);
		*(mbar + MCFSIM_AVR) |= vec;
		mcf_setimr(mcf_getimr() & ~vec);
	}
}

/***************************************************************************/

void mcf_settimericr(unsigned int timer, unsigned int level)
{
	volatile unsigned char *icrp;
	unsigned int icr, imr;

	if (timer <= 2) {
		switch (timer) {
		case 2:  icr = MCFSIM_TIMER2ICR; imr = MCFSIM_IMR_TIMER2; break;
		default: icr = MCFSIM_TIMER1ICR; imr = MCFSIM_IMR_TIMER1; break;
		}

		icrp = (volatile unsigned char *) (MCF_MBAR + icr);
		*icrp = MCFSIM_ICR_AUTOVEC | (level << 2) | MCFSIM_ICR_PRI3;
		mcf_setimr(mcf_getimr() & ~imr);
	}
}

/***************************************************************************/

void m5307_cpu_reset(void)
{
	local_irq_disable();
	/* Set watchdog to soft reset, and enabled */
	__raw_writeb(0xc0, MCF_MBAR + MCFSIM_SYPCR);
	for (;;)
		/* wait for watchdog to timeout */;
}

/***************************************************************************/

void __init config_BSP(char *commandp, int size)
{
	mcf_setimr(MCFSIM_IMR_MASKALL);

#if defined(CONFIG_NETtel) || \
    defined(CONFIG_SECUREEDGEMP3) || defined(CONFIG_CLEOPATRA)
	/* Copy command line from FLASH to local buffer... */
	memcpy(commandp, (char *) 0xf0004000, size);
	commandp[size-1] = 0;
	/* Different timer setup - to prevent device clash */
	mcf_timervector = 30;
	mcf_profilevector = 31;
	mcf_timerlevel = 6;
#endif

	mach_reset = m5307_cpu_reset;

#ifdef CONFIG_BDM_DISABLE
	/*
	 * Disable the BDM clocking.  This also turns off most of the rest of
	 * the BDM device.  This is good for EMC reasons. This option is not
	 * incompatible with the memory protection option.
	 */
	wdebug(MCFDEBUG_CSR, MCFDEBUG_CSR_PSTCLK);
#endif
}

/***************************************************************************/

static int __init init_BSP(void)
{
	m5307_uarts_init();
	platform_add_devices(m5307_devices, ARRAY_SIZE(m5307_devices));
	return 0;
}

arch_initcall(init_BSP);

/***************************************************************************/
