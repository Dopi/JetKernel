/*
 * arch/arm/mach-ep93xx/include/mach/system.h
 */

#include <mach/hardware.h>

static inline void arch_idle(void)
{
	cpu_do_idle();
}

static inline void arch_reset(char mode, const char *cmd)
{
	u32 devicecfg;

	local_irq_disable();

	devicecfg = __raw_readl(EP93XX_SYSCON_DEVICE_CONFIG);
	__raw_writel(0xaa, EP93XX_SYSCON_SWLOCK);
	__raw_writel(devicecfg | 0x80000000, EP93XX_SYSCON_DEVICE_CONFIG);
	__raw_writel(0xaa, EP93XX_SYSCON_SWLOCK);
	__raw_writel(devicecfg & ~0x80000000, EP93XX_SYSCON_DEVICE_CONFIG);

	while (1)
		;
}
