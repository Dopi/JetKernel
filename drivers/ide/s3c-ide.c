/* linux/drivers/ide/s3c-ide.c
 *
 * Copyright (C) 2009 Samsung Electronics
 *      http://samsungsemi.com/
 *
 * S5PC1XX IDE Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include <linux/init.h>
#include <linux/ide.h>
#include <linux/sysdev.h>
#include <linux/scatterlist.h>

#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>

#include <asm/dma.h>
#include <asm/dma-mapping.h>

#include <plat/regs-ide.h>
#include <plat/regs-gpio.h>
#include <plat/regs-clock.h>

#include <asm/io.h>
#include "s3c-ide.h"

#define DRV_NAME	"s3c-ide"

#define DBG_ATA
#undef DBG_ATA

#ifdef DBG_ATA
#define DbgAta(x...) printk(x)
#else
#define DbgAta(x...) do {} while(0);
#endif

#define STATUS_DEVICE_BUSY	0x80

/* Global Declarations */
static s3c_ide_hwif_t s3c_ide_hwif;
static void __iomem *s3c_ide_regbase;
static struct resource *s3c_ide_res;
static struct clk *s3cide_clock;

static inline int bus_fifo_status_check (BUS_STATE status)
{
        u32 temp;
        uint i = 0x10000000;

        switch (status) {
        case IDLE:
                do {
                        temp = readl(s3c_ide_regbase + S5P_BUS_FIFO_STATUS);
                        if (temp == 0)
                                return 0;
                        i--;
                } while (i);
                break;

        /* write case */
        case PAUSER2:
                do {
                        temp = readl(s3c_ide_regbase + S5P_BUS_FIFO_STATUS);
                        if ((temp>>16) == 0x6)
                                return 0;
                        i--;
                } while (i);
                break;

        /* read case */
        case PAUSEW:
                do {
                        temp = readl(s3c_ide_regbase + S5P_BUS_FIFO_STATUS);
                        if ((temp>>16) == 0x5)
                                return 0;
                        i--;
                } while (i);
                break;

        default:
                printk("unknown case to go\n");
                return -1;
        }

        printk("i = %d\n", i);
        if (i == 0)
                printk("we got a problem in bus_fifo\n");
        return temp;
}

#ifdef CONFIG_BLK_DEV_IDE_S3C_UDMA
static int wait_for_dev_ready (void)
{
	u8 temp;
	uint i;

	for (i=0; i<1000000; i++) {
		temp = readl(s3c_ide_regbase + S5P_ATA_PIO_CSD);
		if ((temp & STATUS_DEVICE_BUSY) == 0) {
			DbgAta("wait_for_dev_ready: %08x\n", temp);
			return 0;
		}
	}
	printk("wait_for_dev_ready: error\n");
	return -1;
}

static inline int ata_status_check (ide_drive_t * drive, u8 startend)
{
	u8 stat;
	uint i;

	for (i=0; i<10000000; i++) {
		stat = readl(s3c_ide_regbase + S5P_ATA_PIO_CSD);
		/* Starting DMA */
		if ((stat == 0x58) && (startend == 0))     
			return 0;
		/* Terminating DMA */
		else if ((stat == 0x50) && (startend == 1))
			return 0;
	}
	printk("got error in ata_status_check : %08x\n", stat);
	return -1;
}
#endif


/* Set ATA Mode */
#if defined(CONFIG_BLK_DEV_IDE_S3C_UDMA)
static void set_config_mode (ATA_MODE mode, int rw)
{
        u32 reg = readl(s3c_ide_regbase + S5P_ATA_CFG) & ~(0x39c);

        /* set mode */
        reg |= mode << 2;

        /* DMA write mode */
        if (mode && rw)
                reg |= 0x10;

        /* set ATA DMA auto mode (enable multi block transfer) */
        if (mode == UDMA)
                reg |= 0x380;

        writel(reg, s3c_ide_regbase + S5P_ATA_CFG);
        DbgAta("S5P_ATA_CFG = 0x%08x\n", readl(s3c_ide_regbase + S5P_ATA_CFG));
}
#endif

static void set_ata_enable (uint on)
{
	u32 temp;

	temp = readl(s3c_ide_regbase + S5P_ATA_CTRL);

	if (on)
		writel(temp | 0x1, s3c_ide_regbase + S5P_ATA_CTRL);
	else
		writel(temp & 0xfffffffe, s3c_ide_regbase + S5P_ATA_CTRL);
}

static void set_endian_mode (uint little)
{
	u32 reg = readl(s3c_ide_regbase + S5P_ATA_CFG);

	/* set Little endian */
	if (little)
		reg &= (~0x40);
	else
		reg |= 0x40;

	writel(reg, s3c_ide_regbase + S5P_ATA_CFG);
}


static inline void set_trans_command (ATA_TRANSFER_CMD cmd)
{
	writel(cmd, s3c_ide_regbase + S5P_ATA_CMD);
}

/* Rate filter for PIO and UDMA modes */
static u8 s3c_ide_ratefilter (ide_drive_t *drive, u8 speed)
{
	if (drive->media != ide_disk)
		return min(speed, (u8)XFER_PIO_4);

	switch(speed) {
	case XFER_UDMA_6:
	case XFER_UDMA_5:
	/* S5PC1OO can support upto UDMA4 */
		speed = XFER_UDMA_4;
		break;
	case XFER_UDMA_4:
	case XFER_UDMA_3:
	case XFER_UDMA_2:
	case XFER_UDMA_1:
	case XFER_UDMA_0:
		break;
	default:
		speed = min(speed, (u8)XFER_PIO_4);
		break;
	}
	return speed;
}

static void s3c_ide_tune_chipset (ide_drive_t * drive, u8 xferspeed)
{
	s3c_ide_hwif_t *s3c_hwif = &s3c_ide_hwif;

	u8 speed = s3c_ide_ratefilter(drive, xferspeed);

	/* IORDY is enabled for modes > PIO2 */
	if (XFER_PIO_0 <= speed && speed <= XFER_PIO_4) {
		ulong ata_cfg = readl(s3c_ide_regbase + S5P_ATA_CFG);

		switch (speed) {
		case XFER_PIO_0:
		case XFER_PIO_1:
		case XFER_PIO_2:
			ata_cfg &= (~0x2);
			break;
		case XFER_PIO_3:
		case XFER_PIO_4:
			ata_cfg |= 0x2;
			break;
		}

		writel(ata_cfg, s3c_ide_regbase + S5P_ATA_CFG);
		writel(s3c_hwif->piotime[speed-XFER_PIO_0], s3c_ide_regbase + S5P_ATA_PIO_TIME);
	} else {
		writel(s3c_hwif->piotime[0], s3c_ide_regbase + S5P_ATA_PIO_TIME);
		writel(s3c_hwif->udmatime[speed-XFER_UDMA_0], s3c_ide_regbase + S5P_ATA_UDMA_TIME);
	}
	ide_config_drive_speed(drive, speed);
}


int s3c_ide_ack_intr (ide_hwif_t *hwif)
{
#ifdef CONFIG_BLK_DEV_IDE_S3C_UDMA
	s3c_ide_hwif_t *s3c_hwif = (s3c_ide_hwif_t *) hwif->hwif_data;
#endif
	u32 reg;

	reg = readl(s3c_ide_regbase + S5P_ATA_IRQ);
	DbgAta("S5P_ATA_IRQ: %08x\n", reg);

#ifdef CONFIG_BLK_DEV_IDE_S3C_UDMA
	s3c_hwif->irq_sta = reg;
#endif

	return 1;
}

static void s3c_ide_tune_drive (ide_drive_t * drive, u8 pio)
{
	writel(0x3f, s3c_ide_regbase + S5P_ATA_IRQ);
	writel(0x3f, s3c_ide_regbase + S5P_ATA_IRQ_MSK);
	pio = ide_get_best_pio_mode(drive, 255, pio);
	(void) s3c_ide_tune_chipset(drive, (XFER_PIO_0 + pio));
}

#ifdef CONFIG_BLK_DEV_IDE_S3C_UDMA
/* Dummy function as required by DMA OPS */
static void s3c_ide_dma_host_set (ide_drive_t * drive, int on)
{
	DbgAta("##### %s\n", __FUNCTION__);
}

static int s3c_ide_build_sglist (ide_drive_t * drive, struct request *rq)
{
	ide_hwif_t *hwif = drive->hwif;
	s3c_ide_hwif_t *s3c_hwif = (s3c_ide_hwif_t *) hwif->hwif_data;
	struct scatterlist *sg = hwif->sg_table;

	ide_map_sg(drive, rq);

	if (rq_data_dir(rq) == READ)
		hwif->sg_dma_direction = DMA_FROM_DEVICE;
	else
		hwif->sg_dma_direction = DMA_TO_DEVICE;

	return dma_map_sg(&s3c_hwif->dev, sg, hwif->sg_nents, hwif->sg_dma_direction);
}

/* Building the Scatter Gather Table */
static int s3c_ide_build_dmatable (ide_drive_t * drive)
{
	int i, size, count = 0;
	ide_hwif_t *hwif = drive->hwif;
	struct request *rq = HWGROUP(drive)->rq;
	s3c_ide_hwif_t *s3c_hwif = (s3c_ide_hwif_t *) hwif->hwif_data;
	struct scatterlist *sg;

	u32 addr_reg, size_reg;

	if (rq_data_dir(rq) == WRITE) {
		addr_reg = S5P_ATA_SBUF_START;
		size_reg = S5P_ATA_SBUF_SIZE;
	} else {
		addr_reg = S5P_ATA_TBUF_START;
		size_reg = S5P_ATA_TBUF_SIZE;
	}

	/* Save for interrupt context */
	s3c_hwif->drive = drive;

	/* Build sglist */
	hwif->sg_nents = size = s3c_ide_build_sglist(drive, rq);
	DbgAta("hwif->sg_nents %d\n", hwif->sg_nents);

	if (size > 1) {
		s3c_hwif->pseudo_dma = 1;
		DbgAta("sg_nents is more than 1: %d\n", hwif->sg_nents);
	}
	if (!size)
		return 0;

	/* fill the descriptors */
	sg = hwif->sg_table;
	for (i=0, sg = hwif->sg_table; i<size && sg_dma_len(sg); i++, sg++) {
		s3c_hwif->table[i].addr = sg_dma_address(sg);
		s3c_hwif->table[i].len = sg_dma_len(sg);
		count += s3c_hwif->table[i].len;
		DbgAta("data: %08lx %08lx\n",
			s3c_hwif->table[i].addr,
			s3c_hwif->table[i].len);
	}
	s3c_hwif->table[i].addr = 0;
	s3c_hwif->table[i].len = 0;
	s3c_hwif->queue_size = i;
	DbgAta("total size: %08x, %d\n", count, s3c_hwif->queue_size);

	DbgAta("rw: %08lx %08lx\n",
		s3c_hwif->table[0].addr,
		s3c_hwif->table[0].len);
	writel(s3c_hwif->table[0].len-0x1, s3c_ide_regbase + size_reg);
	writel(s3c_hwif->table[0].addr, s3c_ide_regbase + addr_reg);

	s3c_hwif->index = 1;

	writel(count-1, s3c_ide_regbase + S5P_ATA_XFR_NUM);

	return 1;
}

static int s3c_ide_dma_setup (ide_drive_t * drive)
{
	struct request *rq = HWGROUP(drive)->rq;

	if (!s3c_ide_build_dmatable(drive)) {
		ide_map_sg(drive, rq);
		return 1;
	}

	drive->waiting_for_dma = 1;
	return 0;
}

static void s3c_ide_dma_exec_cmd (ide_drive_t * drive, u8 command)
{
	/* issue cmd to drive */
	ide_execute_command(drive, command, &ide_dma_intr, (WAIT_CMD), NULL);
}

static void s3c_ide_dma_start (ide_drive_t * drive)
{
	struct request *rq = HWGROUP(drive)->rq;
	uint rw = (rq_data_dir(rq) == WRITE);

	ata_status_check(drive, 0);
	writel(0x3ff, s3c_ide_regbase + S5P_ATA_IRQ);
	writel(0x3ff, s3c_ide_regbase + S5P_ATA_IRQ_MSK);

	wait_for_dev_ready();
	bus_fifo_status_check(IDLE);

	set_config_mode(UDMA, rw);
	set_trans_command(ATA_CMD_START);
	return;
}

static int s3c_ide_dma_end (ide_drive_t * drive)
{
	u32 stat = 0;
	ide_hwif_t *hwif = drive->hwif;
	s3c_ide_hwif_t *s3c_hwif = (s3c_ide_hwif_t *) hwif->hwif_data;

	DbgAta("data left: %08x, %08x\n", readl(s3c_ide_regbase + S5P_ATA_XFR_CNT),
				readl(s3c_ide_regbase + S5P_ATA_XFR_NUM));

	stat = readl(s3c_ide_regbase + S5P_BUS_FIFO_STATUS);
	if ( (stat >> 16) == 0x5 ) { /* in case of PAUSEW. */
		writel(ATA_CMD_CONTINUE, s3c_ide_regbase + S5P_ATA_CMD);
	}

	bus_fifo_status_check(IDLE);
	writel(0x3ff, s3c_ide_regbase + S5P_ATA_IRQ_MSK);
	set_config_mode(PIO_CPU, 0);
	ata_status_check(drive, 1);

	drive->waiting_for_dma = 0;

	if (hwif->sg_nents) {
		dma_unmap_sg(&s3c_hwif->dev, hwif->sg_table, hwif->sg_nents,
			     hwif->sg_dma_direction);
		hwif->sg_nents = 0;
	}
	return 0;
}

static int s3c_ide_dma_test_irq (ide_drive_t * drive)
{
	return 1;
}

static void s3c_ide_dma_lostirq (ide_drive_t * drive)
{
	printk("%08x, %08x\n", readl(s3c_ide_regbase + S5P_ATA_IRQ),
				readl(s3c_ide_regbase + S5P_ATA_IRQ_MSK));
	printk("left: %08x, %08x\n", readl(s3c_ide_regbase + S5P_ATA_XFR_CNT),
				readl(s3c_ide_regbase + S5P_ATA_XFR_NUM));

	printk(KERN_ERR "%s: IRQ lost\n", drive->name);
}
#endif

int s3c_ide_irq_hook (void * data)
{
#ifdef CONFIG_BLK_DEV_IDE_S3C_UDMA
	u32 stat = 0, i;
	s3c_ide_hwif_t *s3c_hwif = &s3c_ide_hwif;
#endif

	u32 reg = readl(s3c_ide_regbase + S5P_ATA_IRQ);
	writel(reg, s3c_ide_regbase + S5P_ATA_IRQ);

#ifdef CONFIG_BLK_DEV_IDE_S3C_UDMA
	/* keep transfering data
	 * thus general ide_intr will be ignored. */
	if (s3c_hwif->pseudo_dma) {
		uint i;
		i = s3c_hwif->index;

		if (reg & 0x10) {
			DbgAta("write: %08lx %08lx\n",
				s3c_hwif->table[i].addr,
				s3c_hwif->table[i].len);
			bus_fifo_status_check(PAUSER2);
			writel(s3c_hwif->table[i].len-0x1, s3c_ide_regbase + S5P_ATA_SBUF_SIZE);
			writel(s3c_hwif->table[i].addr, s3c_ide_regbase + S5P_ATA_SBUF_START);
		} else if (reg & 0x08) {
			DbgAta("read: %08lx %08lx\n",
				s3c_hwif->table[i].addr,
				s3c_hwif->table[i].len);

			bus_fifo_status_check(PAUSEW);

			writel(s3c_hwif->table[i].len-0x1, s3c_ide_regbase + S5P_ATA_TBUF_SIZE);
			writel(s3c_hwif->table[i].addr, s3c_ide_regbase + S5P_ATA_TBUF_START);
		} else {
			DbgAta("unexpected interrupt : 0x%x\n", reg);
			return 1;
		}

		s3c_hwif->index++;
		if (s3c_hwif->queue_size == s3c_hwif->index) {
			s3c_hwif->pseudo_dma = 0;
			DbgAta("UDMA close : s3c_hwif->queue_size == s3c_hwif->index\n");
		}

		writel(ATA_CMD_CONTINUE, s3c_ide_regbase + S5P_ATA_CMD);
		return 1;
	}

	for (i=0; i<100000; i++) {
		stat = readl(s3c_ide_regbase + S5P_BUS_FIFO_STATUS);
		if (stat == 0)
			break;
	}
	if (i == 100000)
		printk("BUS has a problem\n");

#endif
	return 0;
}
EXPORT_SYMBOL(s3c_ide_irq_hook);

static void s3c_ide_setup_timing_value (s3c_ide_hwif_t * s3c_hwif)
{
	uint t1, t2, teoc, i;
	ulong uTdvh1, uTdvs, uTrp, uTss, uTackenv;
        uint pio_t1[5] = {70, 50, 30, 30, 25};
        uint pio_t2[5] = {290, 290, 290, 80, 70};
        uint pio_teoc[5] = {240, 43, 10, 70, 25};

        uint uUdmaTdvh[5] = { 50, 32, 29, 25, 24 };
        uint uUdmaTdvs[5] = { 70, 48, 31, 20, 7 };
	uint uUdmaTrp[5] = { 160, 125, 100, 100, 100 };
	uint uUdmaTss[5] = { 50, 50, 50, 50, 50 };
	uint uUdmaTackenvMin[5] = { 20, 20, 20, 20, 20 };

	ulong cycle_time = (uint) (1000000000 / clk_get_rate(s3cide_clock));
	DbgAta("cycle_time = %08lx\n",cycle_time);

	for (i = 0; i < 5; i++) {
		t1 = (pio_t1[i] / cycle_time) & 0x0f;
		t2 = (pio_t2[i] / cycle_time) & 0xff;
		teoc = (pio_teoc[i] / cycle_time) & 0xff;
		s3c_hwif->piotime[i] = (teoc << 12) | (t2 << 4) | t1;
		DbgAta("PIO%dTIME = %08lx\n", i, s3c_hwif->piotime[i]);
	}

	for (i = 0; i < 5; i++) {
		uTdvh1 = (uUdmaTdvh[i] / cycle_time) & 0x0f;
		uTdvs = (uUdmaTdvs[i] / cycle_time) & 0xff;
		uTrp = (uUdmaTrp[i] / cycle_time) & 0xff;
		uTss = (uUdmaTss[i] / cycle_time) & 0x0f;
		uTackenv = (uUdmaTackenvMin[i] / cycle_time) & 0x0f;
		s3c_hwif->udmatime[i] = (uTdvh1 << 24) | (uTdvs << 16) | (uTrp << 8) | (uTss << 4) | uTackenv;
		DbgAta("UDMA%dTIME = %08lx\n", i, s3c_hwif->udmatime[i]);
	}
}

static void __devinit s3c_ide_setup_ports (hw_regs_t *hw, s3c_ide_hwif_t *s3c_hwif)
{
        int i;
        unsigned long *ata_regs = hw->io_ports_array;

        for (i = 0; i < 9; i++)
                *ata_regs++ = (ulong)s3c_ide_regbase + 0x1954 + (i << 2);
}

static void change_mode_to_ata (void)
{
	/* Output pad disable, Card power off, ATA mode */
	writel(0x07, s3c_ide_regbase + S5P_CFATA_MUX);
	mdelay(500);	/* wait for 500ms */
}

static u8 s3c_cable_detect(ide_hwif_t *hwif)
{
	/* UDMA4=80-pin cable */
	return ATA_CBL_PATA80;
}

static const struct ide_port_ops s3c_port_ops = {
	.set_pio_mode		= s3c_ide_tune_drive,
	.set_dma_mode		= s3c_ide_tune_chipset,
	.cable_detect		= s3c_cable_detect,
};

#ifdef CONFIG_BLK_DEV_IDE_S3C_UDMA
static int s3c_ide_dma_init(ide_hwif_t *hwif,
                                      const struct ide_port_info *d)
{
	hwif->hwif_data = &(s3c_ide_hwif);

	return 0;
}

static const struct ide_dma_ops s3c_dma_ops = {
	.dma_host_set		= s3c_ide_dma_host_set,
	.dma_setup		= s3c_ide_dma_setup,
	.dma_start		= s3c_ide_dma_start,
	.dma_end		= s3c_ide_dma_end,
	.dma_test_irq		= s3c_ide_dma_test_irq,
	.dma_lost_irq		= s3c_ide_dma_lostirq,
	.dma_exec_cmd		= s3c_ide_dma_exec_cmd,
	.dma_timeout		= ide_dma_timeout,
};
#endif

static const struct ide_port_info s3c_port_info = {
	.name		= DRV_NAME,
#ifdef CONFIG_BLK_DEV_IDE_S3C_UDMA
	.init_dma	= s3c_ide_dma_init,
#endif
	.chipset	= ide_s3c,
	.port_ops	= &s3c_port_ops,
/*tp_ops left to the default*/
#ifdef CONFIG_BLK_DEV_IDE_S3C_UDMA
	.dma_ops	= &s3c_dma_ops,
#endif
	.host_flags	= IDE_HFLAG_MMIO | IDE_HFLAG_NO_IO_32BIT | IDE_HFLAG_UNMASK_IRQS,
	.pio_mask	= ATA_PIO4,
	.udma_mask	= ATA_UDMA4,
};

static int __devinit s3c_ide_probe (struct platform_device *pdev)
{
	s3c_ide_hwif_t *s3c_hwif = &s3c_ide_hwif;
	struct ide_host *host;
	int ret = 0;
	unsigned long reg;
	hw_regs_t hw, *hws[] = { &hw, NULL, NULL, NULL };

#ifdef CONFIG_BLK_DEV_IDE_S3C_UDMA
	char *mode = "PIO/UDMA";
#else
	char *mode = "PIO_ONLY";
#endif

	memset(&s3c_ide_hwif, 0, sizeof(s3c_ide_hwif));
	s3c_hwif->irq = platform_get_irq(pdev, 0);

        s3c_ide_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

        if (s3c_ide_res == NULL) {
                pr_debug("%s %d: no base address\n", DRV_NAME, pdev->id);
                ret = -ENODEV;
                goto out;
        }
        if (s3c_hwif->irq < 0) {
                pr_debug("%s %d: no IRQ\n", DRV_NAME, pdev->id);
                ret = -ENODEV;
                goto out;
        }

        if (!request_mem_region(s3c_ide_res->start, s3c_ide_res->end - s3c_ide_res->start + 1,
                                pdev->name)) {
                pr_debug("%s: request_mem_region failed\n", DRV_NAME);
                ret =  -EBUSY;
                goto out;
        }

        s3c_ide_regbase = ioremap(s3c_ide_res->start, s3c_ide_res->end - s3c_ide_res->start + 1);
        if (s3c_ide_regbase == 0) {
                ret = -ENOMEM;
                goto out;
        }

	s3cide_clock = clk_get(&pdev->dev, "cfcon");
	if (IS_ERR(s3cide_clock)) {
		printk("failed to find clock source.\n");
		ret = PTR_ERR(s3cide_clock);
		s3cide_clock = NULL;
		goto out;
	}

	ret = clk_enable(s3cide_clock);
	if (ret) {
		printk("failed to enable clock source.\n");
		goto out;
	}

	s3c_ide_setup_timing_value(s3c_hwif);

	/* GPIO Settings */
	reg = readl(S5P_MEM_SYS_CFG) & ~(0x0000003f);
	writel(reg | 0x00000030, S5P_MEM_SYS_CFG);

	/* CF_ADDR[0:2], CF_IORDY, CF_INTRQ, CF_INPACK, CF_RESET, CF_REG */
	writel(0x44444444, S5PC1XX_GPJ0CON);
	writel(0x0, S5PC1XX_GPJ0PUD);

	/*CF_DATA[7:0] */
	writel(0x44444444, S5PC1XX_GPJ2CON);
	writel(0x0, S5PC1XX_GPJ2PUD);

	/* CF_DATA[15:8] */
	writel(0x44444444, S5PC1XX_GPJ3CON);
	writel(0x0, S5PC1XX_GPJ3PUD);

	/* CF_CS0, CF_CS1, CF_IORD, CF_IOWR */
	reg = readl(S5PC1XX_GPJ4CON) & ~(0xffff);
	writel( reg | 0x4444, S5PC1XX_GPJ4CON);
	reg = readl(S5PC1XX_GPJ4PUD) & ~(0xff);

	/* EBI_OE, EBI_WE */
	reg = readl(S5PC1XX_GPK0CON) & ~(0xff000000);

	/* CF_OE, CF_WE */
	reg = readl(S5PC1XX_GPK1CON) & ~(0xff0000);
	writel( reg | 0x00220000, S5PC1XX_GPK1CON);
	reg = readl(S5PC1XX_GPK1PUD) & ~(0xf00);

	/* CF_CD */
	reg = readl(S5PC1XX_GPK3CON) & ~(0xf00000);
	writel( reg | 0x00200000, S5PC1XX_GPK3CON);
	reg = readl(S5PC1XX_GPK3PUD) & ~(0xc00);
	
	/*Change to true IDE mode */
	change_mode_to_ata();

	/* Initial Card Settings */
        writel(0x1C238, s3c_ide_regbase + S5P_ATA_PIO_TIME);
        writel(0x20B1362, s3c_ide_regbase + S5P_ATA_UDMA_TIME);

	set_endian_mode(0);
	set_ata_enable(1);
	mdelay(100);
	
	/* Remove IRQ Status */
	writel(0x3f, s3c_ide_regbase + S5P_ATA_IRQ);
	writel(0x3f, s3c_ide_regbase + S5P_ATA_IRQ_MSK);

	/* Reserved bit to be fixed to 1 */
	reg = readl(s3c_ide_regbase + S5P_ATA_CFG);
	reg |= (1 << 31);		
	writel(reg, s3c_ide_regbase + S5P_ATA_CFG);	
	
	memset(&hw, 0, sizeof(hw));
	s3c_ide_setup_ports(&hw, s3c_hwif);
	hw.irq = s3c_hwif->irq;
	hw.dev = &pdev->dev;
	
	ret = ide_host_add(&s3c_port_info, hws, &host);
	if (ret)
		goto out;

	s3c_ide_hwif.hwif = host->ports[0];
	
	platform_set_drvdata(pdev, host);
	
	printk(KERN_INFO "S5PC1XX IDE(builtin) configured for %s\n", mode);

out:
	return ret;
}

static int __devexit s3c_ide_remove(struct platform_device *pdev)
{
	struct ide_host *host = platform_get_drvdata(pdev);

        ide_host_remove(host);

	iounmap(s3c_ide_regbase);
        release_resource(s3c_ide_res);
        kfree(s3c_ide_res);

	return 0;
}

#if 0
#ifdef CONFIG_PM
unsigned int ata_cfg; ata_ctrl, ata_piotime, ata_udmatime; 
static int s3c_ide_suspend(struct platform_device *pdev, pm_message_t state)
{
        ata_cfg = readl(s3c_ide_regbase + S5P_ATA_CFG);
        ata_ctrl = readl(s3c_ide_regbase + S5P_ATA_CTRL);
        ata_piotime = readl(s3c_ide_regbase + S5P_ATA_PIO_TIME);
        ata_udmatime = readl(s3c_ide_regbase + S5P_ATA_UDMA_TIME);

	set_trans_command(ATA_CMD_SLEEP);
        disable_irq(IRQ_CFC);
        clk_disable(s3cide_clock);

        return 0;
}

static int s3c_ide_resume(struct platform_device *pdev)
{
	clk_enable(s3cide_clock);
        enable_irq(IRQ_CFC);
	set_trans_command(ATA_CMD_DEV_RESET);

        writel(s3c_ide_regbase + S3C_ATA_CFG);
        writel(s3c_ide_regbase + S3C_ATA_CTRL);
        writel(s3c_ide_regbase + S5P_ATA_PIO_TIME);
        writel(s3c_ide_regbase + S5P_ATA_UDMA_TIME);

        return 0;
}

#else
#define s3c_ide_suspend NULL
#define s3c_ide_resume  NULL
#endif
#endif

static struct platform_driver s3c_ide_driver = {
	.probe = s3c_ide_probe,
	.remove = __devexit_p(s3c_ide_remove),
        //.suspend = s3c_ide_suspend,
        //.resume  = s3c_ide_resume,
        .driver = {
                .name = "s3c-ide",
                .owner = THIS_MODULE,
        },
};


static int __init s3c_ide_init(void)
{
	return platform_driver_register(&s3c_ide_driver);
}

static void __exit s3c_ide_exit(void)
{
	platform_driver_unregister(&s3c_ide_driver);
}

module_init(s3c_ide_init);
module_exit(s3c_ide_exit);

MODULE_DESCRIPTION("Samsung S5PC1XX IDE");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:s3c-cfcon");

