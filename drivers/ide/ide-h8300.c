/*
 * H8/300 generic IDE interface
 */

#include <linux/init.h>
#include <linux/ide.h>

#include <asm/io.h>
#include <asm/irq.h>

#define DRV_NAME "ide-h8300"

#define bswap(d) \
({					\
	u16 r;				\
	__asm__("mov.b %w1,r1h\n\t"	\
		"mov.b %x1,r1l\n\t"	\
		"mov.w r1,%0"		\
		:"=r"(r)		\
		:"r"(d)			\
		:"er1");		\
	(r);				\
})

static void mm_outw(u16 d, unsigned long a)
{
	__asm__("mov.b %w0,r2h\n\t"
		"mov.b %x0,r2l\n\t"
		"mov.w r2,@%1"
		:
		:"r"(d),"r"(a)
		:"er2");
}

static u16 mm_inw(unsigned long a)
{
	register u16 r __asm__("er0");
	__asm__("mov.w @%1,r2\n\t"
		"mov.b r2l,%x0\n\t"
		"mov.b r2h,%w0"
		:"=r"(r)
		:"r"(a)
		:"er2");
	return r;
}

static void h8300_tf_load(ide_drive_t *drive, ide_task_t *task)
{
	ide_hwif_t *hwif = drive->hwif;
	struct ide_io_ports *io_ports = &hwif->io_ports;
	struct ide_taskfile *tf = &task->tf;
	u8 HIHI = (task->tf_flags & IDE_TFLAG_LBA48) ? 0xE0 : 0xEF;

	if (task->tf_flags & IDE_TFLAG_FLAGGED)
		HIHI = 0xFF;

	if (task->tf_flags & IDE_TFLAG_OUT_DATA)
		mm_outw((tf->hob_data << 8) | tf->data, io_ports->data_addr);

	if (task->tf_flags & IDE_TFLAG_OUT_HOB_FEATURE)
		outb(tf->hob_feature, io_ports->feature_addr);
	if (task->tf_flags & IDE_TFLAG_OUT_HOB_NSECT)
		outb(tf->hob_nsect, io_ports->nsect_addr);
	if (task->tf_flags & IDE_TFLAG_OUT_HOB_LBAL)
		outb(tf->hob_lbal, io_ports->lbal_addr);
	if (task->tf_flags & IDE_TFLAG_OUT_HOB_LBAM)
		outb(tf->hob_lbam, io_ports->lbam_addr);
	if (task->tf_flags & IDE_TFLAG_OUT_HOB_LBAH)
		outb(tf->hob_lbah, io_ports->lbah_addr);

	if (task->tf_flags & IDE_TFLAG_OUT_FEATURE)
		outb(tf->feature, io_ports->feature_addr);
	if (task->tf_flags & IDE_TFLAG_OUT_NSECT)
		outb(tf->nsect, io_ports->nsect_addr);
	if (task->tf_flags & IDE_TFLAG_OUT_LBAL)
		outb(tf->lbal, io_ports->lbal_addr);
	if (task->tf_flags & IDE_TFLAG_OUT_LBAM)
		outb(tf->lbam, io_ports->lbam_addr);
	if (task->tf_flags & IDE_TFLAG_OUT_LBAH)
		outb(tf->lbah, io_ports->lbah_addr);

	if (task->tf_flags & IDE_TFLAG_OUT_DEVICE)
		outb((tf->device & HIHI) | drive->select,
		     io_ports->device_addr);
}

static void h8300_tf_read(ide_drive_t *drive, ide_task_t *task)
{
	ide_hwif_t *hwif = drive->hwif;
	struct ide_io_ports *io_ports = &hwif->io_ports;
	struct ide_taskfile *tf = &task->tf;

	if (task->tf_flags & IDE_TFLAG_IN_DATA) {
		u16 data = mm_inw(io_ports->data_addr);

		tf->data = data & 0xff;
		tf->hob_data = (data >> 8) & 0xff;
	}

	/* be sure we're looking at the low order bits */
	outb(ATA_DEVCTL_OBS & ~0x80, io_ports->ctl_addr);

	if (task->tf_flags & IDE_TFLAG_IN_FEATURE)
		tf->feature = inb(io_ports->feature_addr);
	if (task->tf_flags & IDE_TFLAG_IN_NSECT)
		tf->nsect  = inb(io_ports->nsect_addr);
	if (task->tf_flags & IDE_TFLAG_IN_LBAL)
		tf->lbal   = inb(io_ports->lbal_addr);
	if (task->tf_flags & IDE_TFLAG_IN_LBAM)
		tf->lbam   = inb(io_ports->lbam_addr);
	if (task->tf_flags & IDE_TFLAG_IN_LBAH)
		tf->lbah   = inb(io_ports->lbah_addr);
	if (task->tf_flags & IDE_TFLAG_IN_DEVICE)
		tf->device = inb(io_ports->device_addr);

	if (task->tf_flags & IDE_TFLAG_LBA48) {
		outb(ATA_DEVCTL_OBS | 0x80, io_ports->ctl_addr);

		if (task->tf_flags & IDE_TFLAG_IN_HOB_FEATURE)
			tf->hob_feature = inb(io_ports->feature_addr);
		if (task->tf_flags & IDE_TFLAG_IN_HOB_NSECT)
			tf->hob_nsect   = inb(io_ports->nsect_addr);
		if (task->tf_flags & IDE_TFLAG_IN_HOB_LBAL)
			tf->hob_lbal    = inb(io_ports->lbal_addr);
		if (task->tf_flags & IDE_TFLAG_IN_HOB_LBAM)
			tf->hob_lbam    = inb(io_ports->lbam_addr);
		if (task->tf_flags & IDE_TFLAG_IN_HOB_LBAH)
			tf->hob_lbah    = inb(io_ports->lbah_addr);
	}
}

static void mm_outsw(unsigned long addr, void *buf, u32 len)
{
	unsigned short *bp = (unsigned short *)buf;
	for (; len > 0; len--, bp++)
		*(volatile u16 *)addr = bswap(*bp);
}

static void mm_insw(unsigned long addr, void *buf, u32 len)
{
	unsigned short *bp = (unsigned short *)buf;
	for (; len > 0; len--, bp++)
		*bp = bswap(*(volatile u16 *)addr);
}

static void h8300_input_data(ide_drive_t *drive, struct request *rq,
			     void *buf, unsigned int len)
{
	mm_insw(drive->hwif->io_ports.data_addr, buf, (len + 1) / 2);
}

static void h8300_output_data(ide_drive_t *drive, struct request *rq,
			      void *buf, unsigned int len)
{
	mm_outsw(drive->hwif->io_ports.data_addr, buf, (len + 1) / 2);
}

static const struct ide_tp_ops h8300_tp_ops = {
	.exec_command		= ide_exec_command,
	.read_status		= ide_read_status,
	.read_altstatus		= ide_read_altstatus,

	.set_irq		= ide_set_irq,

	.tf_load		= h8300_tf_load,
	.tf_read		= h8300_tf_read,

	.input_data		= h8300_input_data,
	.output_data		= h8300_output_data,
};

#define H8300_IDE_GAP (2)

static inline void hw_setup(hw_regs_t *hw)
{
	int i;

	memset(hw, 0, sizeof(hw_regs_t));
	for (i = 0; i <= 7; i++)
		hw->io_ports_array[i] = CONFIG_H8300_IDE_BASE + H8300_IDE_GAP*i;
	hw->io_ports.ctl_addr = CONFIG_H8300_IDE_ALT;
	hw->irq = EXT_IRQ0 + CONFIG_H8300_IDE_IRQ;
	hw->chipset = ide_generic;
}

static const struct ide_port_info h8300_port_info = {
	.tp_ops			= &h8300_tp_ops,
	.host_flags		= IDE_HFLAG_NO_IO_32BIT | IDE_HFLAG_NO_DMA,
};

static int __init h8300_ide_init(void)
{
	hw_regs_t hw, *hws[] = { &hw, NULL, NULL, NULL };

	printk(KERN_INFO DRV_NAME ": H8/300 generic IDE interface\n");

	if (!request_region(CONFIG_H8300_IDE_BASE, H8300_IDE_GAP*8, "ide-h8300"))
		goto out_busy;
	if (!request_region(CONFIG_H8300_IDE_ALT, H8300_IDE_GAP, "ide-h8300")) {
		release_region(CONFIG_H8300_IDE_BASE, H8300_IDE_GAP*8);
		goto out_busy;
	}

	hw_setup(&hw);

	return ide_host_add(&h8300_port_info, hws, NULL);

out_busy:
	printk(KERN_ERR "ide-h8300: IDE I/F resource already used.\n");

	return -EBUSY;
}

module_init(h8300_ide_init);

MODULE_LICENSE("GPL");
