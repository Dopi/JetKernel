/*
 * 8259 interrupt controller emulation
 *
 * Copyright (c) 2003-2004 Fabrice Bellard
 * Copyright (c) 2007 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * Authors:
 *   Yaozu (Eddie) Dong <Eddie.dong@intel.com>
 *   Port from Qemu.
 */
#include <linux/mm.h>
#include <linux/bitops.h>
#include "irq.h"

#include <linux/kvm_host.h>

static void pic_lock(struct kvm_pic *s)
{
	spin_lock(&s->lock);
}

static void pic_unlock(struct kvm_pic *s)
{
	struct kvm *kvm = s->kvm;
	unsigned acks = s->pending_acks;
	bool wakeup = s->wakeup_needed;
	struct kvm_vcpu *vcpu;

	s->pending_acks = 0;
	s->wakeup_needed = false;

	spin_unlock(&s->lock);

	while (acks) {
		kvm_notify_acked_irq(kvm, __ffs(acks));
		acks &= acks - 1;
	}

	if (wakeup) {
		vcpu = s->kvm->vcpus[0];
		if (vcpu)
			kvm_vcpu_kick(vcpu);
	}
}

static void pic_clear_isr(struct kvm_kpic_state *s, int irq)
{
	s->isr &= ~(1 << irq);
	s->isr_ack |= (1 << irq);
}

void kvm_pic_clear_isr_ack(struct kvm *kvm)
{
	struct kvm_pic *s = pic_irqchip(kvm);
	s->pics[0].isr_ack = 0xff;
	s->pics[1].isr_ack = 0xff;
}

/*
 * set irq level. If an edge is detected, then the IRR is set to 1
 */
static inline void pic_set_irq1(struct kvm_kpic_state *s, int irq, int level)
{
	int mask;
	mask = 1 << irq;
	if (s->elcr & mask)	/* level triggered */
		if (level) {
			s->irr |= mask;
			s->last_irr |= mask;
		} else {
			s->irr &= ~mask;
			s->last_irr &= ~mask;
		}
	else	/* edge triggered */
		if (level) {
			if ((s->last_irr & mask) == 0)
				s->irr |= mask;
			s->last_irr |= mask;
		} else
			s->last_irr &= ~mask;
}

/*
 * return the highest priority found in mask (highest = smallest
 * number). Return 8 if no irq
 */
static inline int get_priority(struct kvm_kpic_state *s, int mask)
{
	int priority;
	if (mask == 0)
		return 8;
	priority = 0;
	while ((mask & (1 << ((priority + s->priority_add) & 7))) == 0)
		priority++;
	return priority;
}

/*
 * return the pic wanted interrupt. return -1 if none
 */
static int pic_get_irq(struct kvm_kpic_state *s)
{
	int mask, cur_priority, priority;

	mask = s->irr & ~s->imr;
	priority = get_priority(s, mask);
	if (priority == 8)
		return -1;
	/*
	 * compute current priority. If special fully nested mode on the
	 * master, the IRQ coming from the slave is not taken into account
	 * for the priority computation.
	 */
	mask = s->isr;
	if (s->special_fully_nested_mode && s == &s->pics_state->pics[0])
		mask &= ~(1 << 2);
	cur_priority = get_priority(s, mask);
	if (priority < cur_priority)
		/*
		 * higher priority found: an irq should be generated
		 */
		return (priority + s->priority_add) & 7;
	else
		return -1;
}

/*
 * raise irq to CPU if necessary. must be called every time the active
 * irq may change
 */
static void pic_update_irq(struct kvm_pic *s)
{
	int irq2, irq;

	irq2 = pic_get_irq(&s->pics[1]);
	if (irq2 >= 0) {
		/*
		 * if irq request by slave pic, signal master PIC
		 */
		pic_set_irq1(&s->pics[0], 2, 1);
		pic_set_irq1(&s->pics[0], 2, 0);
	}
	irq = pic_get_irq(&s->pics[0]);
	if (irq >= 0)
		s->irq_request(s->irq_request_opaque, 1);
	else
		s->irq_request(s->irq_request_opaque, 0);
}

void kvm_pic_update_irq(struct kvm_pic *s)
{
	pic_lock(s);
	pic_update_irq(s);
	pic_unlock(s);
}

void kvm_pic_set_irq(void *opaque, int irq, int level)
{
	struct kvm_pic *s = opaque;

	pic_lock(s);
	if (irq >= 0 && irq < PIC_NUM_PINS) {
		pic_set_irq1(&s->pics[irq >> 3], irq & 7, level);
		pic_update_irq(s);
	}
	pic_unlock(s);
}

/*
 * acknowledge interrupt 'irq'
 */
static inline void pic_intack(struct kvm_kpic_state *s, int irq)
{
	s->isr |= 1 << irq;
	if (s->auto_eoi) {
		if (s->rotate_on_auto_eoi)
			s->priority_add = (irq + 1) & 7;
		pic_clear_isr(s, irq);
	}
	/*
	 * We don't clear a level sensitive interrupt here
	 */
	if (!(s->elcr & (1 << irq)))
		s->irr &= ~(1 << irq);
}

int kvm_pic_read_irq(struct kvm *kvm)
{
	int irq, irq2, intno;
	struct kvm_pic *s = pic_irqchip(kvm);

	pic_lock(s);
	irq = pic_get_irq(&s->pics[0]);
	if (irq >= 0) {
		pic_intack(&s->pics[0], irq);
		if (irq == 2) {
			irq2 = pic_get_irq(&s->pics[1]);
			if (irq2 >= 0)
				pic_intack(&s->pics[1], irq2);
			else
				/*
				 * spurious IRQ on slave controller
				 */
				irq2 = 7;
			intno = s->pics[1].irq_base + irq2;
			irq = irq2 + 8;
		} else
			intno = s->pics[0].irq_base + irq;
	} else {
		/*
		 * spurious IRQ on host controller
		 */
		irq = 7;
		intno = s->pics[0].irq_base + irq;
	}
	pic_update_irq(s);
	pic_unlock(s);
	kvm_notify_acked_irq(kvm, irq);

	return intno;
}

void kvm_pic_reset(struct kvm_kpic_state *s)
{
	int irq, irqbase, n;
	struct kvm *kvm = s->pics_state->irq_request_opaque;
	struct kvm_vcpu *vcpu0 = kvm->vcpus[0];

	if (s == &s->pics_state->pics[0])
		irqbase = 0;
	else
		irqbase = 8;

	for (irq = 0; irq < PIC_NUM_PINS/2; irq++) {
		if (vcpu0 && kvm_apic_accept_pic_intr(vcpu0))
			if (s->irr & (1 << irq) || s->isr & (1 << irq)) {
				n = irq + irqbase;
				s->pics_state->pending_acks |= 1 << n;
			}
	}
	s->last_irr = 0;
	s->irr = 0;
	s->imr = 0;
	s->isr = 0;
	s->isr_ack = 0xff;
	s->priority_add = 0;
	s->irq_base = 0;
	s->read_reg_select = 0;
	s->poll = 0;
	s->special_mask = 0;
	s->init_state = 0;
	s->auto_eoi = 0;
	s->rotate_on_auto_eoi = 0;
	s->special_fully_nested_mode = 0;
	s->init4 = 0;
}

static void pic_ioport_write(void *opaque, u32 addr, u32 val)
{
	struct kvm_kpic_state *s = opaque;
	int priority, cmd, irq;

	addr &= 1;
	if (addr == 0) {
		if (val & 0x10) {
			kvm_pic_reset(s);	/* init */
			/*
			 * deassert a pending interrupt
			 */
			s->pics_state->irq_request(s->pics_state->
						   irq_request_opaque, 0);
			s->init_state = 1;
			s->init4 = val & 1;
			if (val & 0x02)
				printk(KERN_ERR "single mode not supported");
			if (val & 0x08)
				printk(KERN_ERR
				       "level sensitive irq not supported");
		} else if (val & 0x08) {
			if (val & 0x04)
				s->poll = 1;
			if (val & 0x02)
				s->read_reg_select = val & 1;
			if (val & 0x40)
				s->special_mask = (val >> 5) & 1;
		} else {
			cmd = val >> 5;
			switch (cmd) {
			case 0:
			case 4:
				s->rotate_on_auto_eoi = cmd >> 2;
				break;
			case 1:	/* end of interrupt */
			case 5:
				priority = get_priority(s, s->isr);
				if (priority != 8) {
					irq = (priority + s->priority_add) & 7;
					pic_clear_isr(s, irq);
					if (cmd == 5)
						s->priority_add = (irq + 1) & 7;
					pic_update_irq(s->pics_state);
				}
				break;
			case 3:
				irq = val & 7;
				pic_clear_isr(s, irq);
				pic_update_irq(s->pics_state);
				break;
			case 6:
				s->priority_add = (val + 1) & 7;
				pic_update_irq(s->pics_state);
				break;
			case 7:
				irq = val & 7;
				s->priority_add = (irq + 1) & 7;
				pic_clear_isr(s, irq);
				pic_update_irq(s->pics_state);
				break;
			default:
				break;	/* no operation */
			}
		}
	} else
		switch (s->init_state) {
		case 0:		/* normal mode */
			s->imr = val;
			pic_update_irq(s->pics_state);
			break;
		case 1:
			s->irq_base = val & 0xf8;
			s->init_state = 2;
			break;
		case 2:
			if (s->init4)
				s->init_state = 3;
			else
				s->init_state = 0;
			break;
		case 3:
			s->special_fully_nested_mode = (val >> 4) & 1;
			s->auto_eoi = (val >> 1) & 1;
			s->init_state = 0;
			break;
		}
}

static u32 pic_poll_read(struct kvm_kpic_state *s, u32 addr1)
{
	int ret;

	ret = pic_get_irq(s);
	if (ret >= 0) {
		if (addr1 >> 7) {
			s->pics_state->pics[0].isr &= ~(1 << 2);
			s->pics_state->pics[0].irr &= ~(1 << 2);
		}
		s->irr &= ~(1 << ret);
		pic_clear_isr(s, ret);
		if (addr1 >> 7 || ret != 2)
			pic_update_irq(s->pics_state);
	} else {
		ret = 0x07;
		pic_update_irq(s->pics_state);
	}

	return ret;
}

static u32 pic_ioport_read(void *opaque, u32 addr1)
{
	struct kvm_kpic_state *s = opaque;
	unsigned int addr;
	int ret;

	addr = addr1;
	addr &= 1;
	if (s->poll) {
		ret = pic_poll_read(s, addr1);
		s->poll = 0;
	} else
		if (addr == 0)
			if (s->read_reg_select)
				ret = s->isr;
			else
				ret = s->irr;
		else
			ret = s->imr;
	return ret;
}

static void elcr_ioport_write(void *opaque, u32 addr, u32 val)
{
	struct kvm_kpic_state *s = opaque;
	s->elcr = val & s->elcr_mask;
}

static u32 elcr_ioport_read(void *opaque, u32 addr1)
{
	struct kvm_kpic_state *s = opaque;
	return s->elcr;
}

static int picdev_in_range(struct kvm_io_device *this, gpa_t addr,
			   int len, int is_write)
{
	switch (addr) {
	case 0x20:
	case 0x21:
	case 0xa0:
	case 0xa1:
	case 0x4d0:
	case 0x4d1:
		return 1;
	default:
		return 0;
	}
}

static void picdev_write(struct kvm_io_device *this,
			 gpa_t addr, int len, const void *val)
{
	struct kvm_pic *s = this->private;
	unsigned char data = *(unsigned char *)val;

	if (len != 1) {
		if (printk_ratelimit())
			printk(KERN_ERR "PIC: non byte write\n");
		return;
	}
	pic_lock(s);
	switch (addr) {
	case 0x20:
	case 0x21:
	case 0xa0:
	case 0xa1:
		pic_ioport_write(&s->pics[addr >> 7], addr, data);
		break;
	case 0x4d0:
	case 0x4d1:
		elcr_ioport_write(&s->pics[addr & 1], addr, data);
		break;
	}
	pic_unlock(s);
}

static void picdev_read(struct kvm_io_device *this,
			gpa_t addr, int len, void *val)
{
	struct kvm_pic *s = this->private;
	unsigned char data = 0;

	if (len != 1) {
		if (printk_ratelimit())
			printk(KERN_ERR "PIC: non byte read\n");
		return;
	}
	pic_lock(s);
	switch (addr) {
	case 0x20:
	case 0x21:
	case 0xa0:
	case 0xa1:
		data = pic_ioport_read(&s->pics[addr >> 7], addr);
		break;
	case 0x4d0:
	case 0x4d1:
		data = elcr_ioport_read(&s->pics[addr & 1], addr);
		break;
	}
	*(unsigned char *)val = data;
	pic_unlock(s);
}

/*
 * callback when PIC0 irq status changed
 */
static void pic_irq_request(void *opaque, int level)
{
	struct kvm *kvm = opaque;
	struct kvm_vcpu *vcpu = kvm->vcpus[0];
	struct kvm_pic *s = pic_irqchip(kvm);
	int irq = pic_get_irq(&s->pics[0]);

	s->output = level;
	if (vcpu && level && (s->pics[0].isr_ack & (1 << irq))) {
		s->pics[0].isr_ack &= ~(1 << irq);
		s->wakeup_needed = true;
	}
}

struct kvm_pic *kvm_create_pic(struct kvm *kvm)
{
	struct kvm_pic *s;
	s = kzalloc(sizeof(struct kvm_pic), GFP_KERNEL);
	if (!s)
		return NULL;
	spin_lock_init(&s->lock);
	s->kvm = kvm;
	s->pics[0].elcr_mask = 0xf8;
	s->pics[1].elcr_mask = 0xde;
	s->irq_request = pic_irq_request;
	s->irq_request_opaque = kvm;
	s->pics[0].pics_state = s;
	s->pics[1].pics_state = s;

	/*
	 * Initialize PIO device
	 */
	s->dev.read = picdev_read;
	s->dev.write = picdev_write;
	s->dev.in_range = picdev_in_range;
	s->dev.private = s;
	kvm_io_bus_register_dev(&kvm->pio_bus, &s->dev);
	return s;
}
