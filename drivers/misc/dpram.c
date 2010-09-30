/****************************************************************************
**
** COPYRIGHT(C) : Samsung Electronics Co.Ltd, 2006-2010 ALL RIGHTS RESERVED
**
**                Onedram Device Driver
**
****************************************************************************/

#define _DEBUG
/* HSDPA DUN & Internal FTP Throughput Support. @LDK@ */
#define _HSDPA_DPRAM

#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/irq.h>
#include <linux/poll.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/hardware.h>

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#endif	/* CONFIG_PROC_FS */

#include "dpram.h"

#define DRIVER_NAME 		"DPRAM"
#define DRIVER_PROC_ENTRY	"driver/dpram"
#define DRIVER_MAJOR_NUM	252

#ifdef _DEBUG
#define _ENABLE_ERROR_DEVICE
#define PRINT_WRITE
#define PRINT_READ
#define PRINT_WRITE_SHORT
#define PRINT_READ_SHORT
#define PRINT_SEND_IRQ
#define PRINT_RECV_IRQ
#define PRINT_HEAD_TAIL
#endif

#ifdef _DEBUG
#define dprintk(s, args...) pr_debug("[OneDRAM] %s:%d - " s, __func__, __LINE__,  ##args)
#else
#define dprintk(s, args...)
#endif	/* _DEBUG */

#define WRITE_TO_DPRAM(dest, src, size) \
	_memcpy((void *)(DPRAM_VBASE + dest), src, size)

#define READ_FROM_DPRAM(dest, src, size) \
	_memcpy(dest, (void *)(DPRAM_VBASE + src), size)

#ifdef _ENABLE_ERROR_DEVICE
#define DPRAM_ERR_MSG_LEN			65
#define DPRAM_ERR_DEVICE			"dpramerr"
#endif	/* _ENABLE_ERROR_DEVICE */

#define IRQ_ONEDRAM_INT_N	IRQ_EINT(0)
#define IRQ_PHONE_ACTIVE	IRQ_EINT(7)

static int onedram_get_semaphore(const char*);
static int return_onedram_semaphore(const char*);
static void send_interrupt_to_phone_with_semaphore(u16 irq_mask);

static void __iomem *dpram_base = 0;
static unsigned int *onedram_sem;
static unsigned int *onedram_mailboxBA;		//send mail
static unsigned int *onedram_mailboxAB;		//received mail

static atomic_t onedram_lock;
static int onedram_lock_with_semaphore(const char*);
static void onedram_release_lock(const char*);
static void dpram_drop_data(dpram_device_t *device);

static int requested_semaphore = 0;
static int unreceived_semaphore = 0;
static int phone_sync = 0;
static int dump_on = 0;

static int dpram_phone_getstatus(void);
#define DPRAM_VBASE dpram_base
static struct tty_driver *dpram_tty_driver;
static dpram_tasklet_data_t dpram_tasklet_data[MAX_INDEX];
static dpram_device_t dpram_table[MAX_INDEX] = {
	{
		.in_head_addr = DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS,
		.in_tail_addr = DPRAM_PHONE2PDA_FORMATTED_TAIL_ADDRESS,
		.in_buff_addr = DPRAM_PHONE2PDA_FORMATTED_BUFFER_ADDRESS,
		.in_buff_size = DPRAM_PHONE2PDA_FORMATTED_BUFFER_SIZE,
		.in_head_saved = 0,
		.in_tail_saved = 0,

		.out_head_addr = DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS,
		.out_tail_addr = DPRAM_PDA2PHONE_FORMATTED_TAIL_ADDRESS,
		.out_buff_addr = DPRAM_PDA2PHONE_FORMATTED_BUFFER_ADDRESS,
		.out_buff_size = DPRAM_PDA2PHONE_FORMATTED_BUFFER_SIZE,
		.out_head_saved = 0,
		.out_tail_saved = 0,

		.mask_req_ack = INT_MASK_REQ_ACK_F,
		.mask_res_ack = INT_MASK_RES_ACK_F,
		.mask_send = INT_MASK_SEND_F,
	},
	{
		.in_head_addr = DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS,
		.in_tail_addr = DPRAM_PHONE2PDA_RAW_TAIL_ADDRESS,
		.in_buff_addr = DPRAM_PHONE2PDA_RAW_BUFFER_ADDRESS,
		.in_buff_size = DPRAM_PHONE2PDA_RAW_BUFFER_SIZE,
		.in_head_saved = 0,
		.in_tail_saved = 0,

		.out_head_addr = DPRAM_PDA2PHONE_RAW_HEAD_ADDRESS,
		.out_tail_addr = DPRAM_PDA2PHONE_RAW_TAIL_ADDRESS,
		.out_buff_addr = DPRAM_PDA2PHONE_RAW_BUFFER_ADDRESS,
		.out_buff_size = DPRAM_PDA2PHONE_RAW_BUFFER_SIZE,
		.out_head_saved = 0,
		.out_tail_saved = 0,

		.mask_req_ack = INT_MASK_REQ_ACK_R,
		.mask_res_ack = INT_MASK_RES_ACK_R,
		.mask_send = INT_MASK_SEND_R,
	},
};

static struct tty_struct *dpram_tty[MAX_INDEX];
static struct ktermios *dpram_termios[MAX_INDEX];
static struct ktermios *dpram_termios_locked[MAX_INDEX];

static void res_ack_tasklet_handler(unsigned long data);
static void fmt_rcv_tasklet_handler(unsigned long data);

static DECLARE_TASKLET(fmt_send_tasklet, fmt_rcv_tasklet_handler, 0);
static DECLARE_TASKLET(raw_send_tasklet, fmt_rcv_tasklet_handler, 0);
static DECLARE_TASKLET(fmt_res_ack_tasklet, res_ack_tasklet_handler,
		(unsigned long)&dpram_table[FORMATTED_INDEX]);
static DECLARE_TASKLET(raw_res_ack_tasklet, res_ack_tasklet_handler,
		(unsigned long)&dpram_table[RAW_INDEX]);

static void semaphore_control_handler(unsigned long data);
static DECLARE_TASKLET(semaphore_control_tasklet, semaphore_control_handler, 0);

#ifdef _ENABLE_ERROR_DEVICE
static unsigned int dpram_err_len;
static char dpram_err_buf[DPRAM_ERR_MSG_LEN];

struct class *dpram_class;

static DECLARE_WAIT_QUEUE_HEAD(dpram_err_wait_q);
static struct fasync_struct *dpram_err_async_q;
//extern void usb_switch_mode(int); REMOVED INORDER TO COMPILE ANDROID OMNIA II
#endif	/* _ENABLE_ERROR_DEVICE */

// 2008.10.20.
//static DECLARE_MUTEX(write_mutex);

/* tty related functions. */
static inline void byte_align(unsigned long dest, unsigned long src)
{
	u16 *p_src;
	volatile u16 *p_dest;

	if (!(dest % 2) && !(src % 2)) {
		p_dest = (u16 *)dest;
		p_src = (u16 *)src;

		*p_dest = (*p_dest & 0xFF00) | (*p_src & 0x00FF);
	}

	else if ((dest % 2) && (src % 2)) {
		p_dest = (u16 *)(dest - 1);
		p_src = (u16 *)(src - 1);

		*p_dest = (*p_dest & 0x00FF) | (*p_src & 0xFF00);
	}

	else if (!(dest % 2) && (src % 2)) {
		p_dest = (u16 *)dest;
		p_src = (u16 *)(src - 1);

		*p_dest = (*p_dest & 0xFF00) | ((*p_src >> 8) & 0x00FF);
	}

	else if ((dest % 2) && !(src % 2)) {
		p_dest = (u16 *)(dest - 1);
		p_src = (u16 *)src;

		*p_dest = (*p_dest & 0x00FF) | ((*p_src << 8) & 0xFF00);
	}

	else {
		dprintk("oops.~\n");
	}
}

static inline void _memcpy(void *p_dest, const void *p_src, int size)
{
	unsigned long dest = (unsigned long)p_dest;
	unsigned long src = (unsigned long)p_src;

	if (!(*onedram_sem)) {
		printk("[OneDRAM] memory access without semaphore!: %d\n", *onedram_sem);
		return;
	}
	if (size <= 0) {
		return;
	}

	if (dest & 1) {
		byte_align(dest, src);
		dest++, src++;
		size--;
	}

	if (size & 1) {
		byte_align(dest + size - 1, src + size - 1);
		size--;
	}

	if (src & 1) {
		unsigned char *s = (unsigned char *)src;
		volatile u16 *d = (unsigned short *)dest;

		size >>= 1;

		while (size--) {
			*d++ = s[0] | (s[1] << 8);
			s += 2;
		}
	}

	else {
		u16 *s = (u16 *)src;
		volatile u16 *d = (unsigned short *)dest;

		size >>= 1;

		while (size--) { *d++ = *s++; }
	}
}

static inline int _memcmp(u8 *dest, u8 *src, int size)
{
	int i = 0;
	if (!(*onedram_sem)) {
		printk("[OneDRAM] memory access without semaphore!: %d\n", *onedram_sem);
		return 1;
	}

	while (i++ < size) {
		if (*(dest + i) != *(src + i)) {
			return 1;
		}
	}

	return 0;
}

static inline int WRITE_TO_DPRAM_VERIFY(u32 dest, void *src, int size)
{
	int cnt = 3;

	while (cnt--) {
		_memcpy((void *)(DPRAM_VBASE + dest), (void *)src, size);

		if (!_memcmp((u8 *)(DPRAM_VBASE + dest), (u8 *)src, size))
			return 0;
	}

	return -1;
}

static inline int READ_FROM_DPRAM_VERIFY(void *dest, u32 src, int size)
{
	int cnt = 3;

	while (cnt--) {
		_memcpy((void *)dest, (void *)(DPRAM_VBASE + src), size);

		if (!_memcmp((u8 *)dest, (u8 *)(DPRAM_VBASE + src), size))
			return 0;
	}

	return -1;
}

#if 0
static void send_interrupt_to_phone(u16 irq_mask)
{
	*onedram_mailboxBA = irq_mask;
#ifdef PRINT_SEND_IRQ
	printk("=====> send IRQ: %x for ack\n", irq_mask);
#endif

}
#endif

static int dpram_write(dpram_device_t *device,
		const unsigned char *buf, int len)
{
	int retval = 0;
	int size = 0;
	u16 head, tail;
	u16 irq_mask = 0;
	//unsigned long flags;

//	down_interruptible(&write_mutex);	
#ifdef PRINT_WRITE
	int i;
	printk("WRITE\n");
	for (i = 0; i < len; i++)	
		printk("%02x ", *((unsigned char *)buf + i));
	printk("\n");
#endif
#ifdef PRINT_WRITE_SHORT
		printk("WRITE: len: %d\n", len);
#endif

	if(!onedram_get_semaphore(__func__)) {
		return -1;
	}
		
	if(onedram_lock_with_semaphore(__func__) < 0) {
		return -1;
	}

	READ_FROM_DPRAM_VERIFY(&head, device->out_head_addr, sizeof(head));
	READ_FROM_DPRAM_VERIFY(&tail, device->out_tail_addr, sizeof(tail));

printk("%s, head: %d, tail: %d\n", __func__, head, tail);

	// +++++++++ head ---------- tail ++++++++++ //
	if (head < tail) {
		size = tail - head - 1;
		size = (len > size) ? size : len;

		WRITE_TO_DPRAM(device->out_buff_addr + head, buf, size);
		retval = size;
	}

	// tail +++++++++++++++ head --------------- //
	else if (tail == 0) {
		size = device->out_buff_size - head - 1;
		size = (len > size) ? size : len;

		WRITE_TO_DPRAM(device->out_buff_addr + head, buf, size);
		retval = size;
	}

	// ------ tail +++++++++++ head ------------ //
	else {
		size = device->out_buff_size - head;
		size = (len > size) ? size : len;
		
		WRITE_TO_DPRAM(device->out_buff_addr + head, buf, size);
		retval = size;

		if (len > retval) {
			size = (len - retval > tail - 1) ? tail - 1 : len - retval;
			
			WRITE_TO_DPRAM(device->out_buff_addr, buf + retval, size);
			retval += size;
		}
	}

	/* @LDK@ calculate new head */
	head = (u16)((head + retval) % device->out_buff_size);
	WRITE_TO_DPRAM_VERIFY(device->out_head_addr, &head, sizeof(head));
	

	device->out_head_saved = head;
	device->out_tail_saved = tail;

	/* @LDK@ send interrupt to the phone, if.. */
	irq_mask = INT_MASK_VALID;

	if (retval > 0)
		irq_mask |= device->mask_send;

	if (len > retval)
		irq_mask |= device->mask_req_ack;

	onedram_release_lock(__func__);
//	send_interrupt_to_phone(irq_mask);
	send_interrupt_to_phone_with_semaphore(irq_mask);
//	up(&write_mutex);
#ifdef PRINT_WRITE_SHORT
	printk("WRITE: return: %d\n", retval);
#endif
	return retval;
	
}

static inline int dpram_tty_insert_data(dpram_device_t *device, const u8 *psrc, u16 size)
{
#define CLUSTER_SEGMENT	1500

	u16 copied_size = 0;
	int retval = 0;
	
#ifdef PRINT_READ
	int i;
		printk("READ: %d\n", size);
		for(i=0; i<size; i++)	
			printk("%02x ", *(psrc+ + i));
		printk("\n");
#endif
#ifdef PRINT_READ_SHORT
	printk("READ: size:  %d\n", size);
#endif

	if (size > CLUSTER_SEGMENT && (device->serial.tty->index == 1)) {
		while (size) {
			copied_size = (size > CLUSTER_SEGMENT) ? CLUSTER_SEGMENT : size;
			tty_insert_flip_string(device->serial.tty, psrc + retval, copied_size);

			size -= copied_size;
			retval += copied_size;
		}

		return retval;
	}

	return tty_insert_flip_string(device->serial.tty, psrc, size);
}

static int dpram_read_fmt(dpram_device_t *device, const u16 non_cmd)
{
	int retval = 0;
	int retval_add = 0;
	int size = 0;
	u16 head, tail;

	if(!*onedram_sem)
		printk("!!!!! %s no sem\n", __func__);

	if(onedram_lock_with_semaphore(__func__) < 0)
		return -1;

	READ_FROM_DPRAM_VERIFY(&head, device->in_head_addr, sizeof(head));
	READ_FROM_DPRAM_VERIFY(&tail, device->in_tail_addr, sizeof(tail));

	printk("=====> %s,  head: %d, tail: %d\n", __func__, head, tail);

	if (head != tail) {
		u16 up_tail = 0;

		// ------- tail ++++++++++++ head -------- //
		if (head > tail) {
			size = head - tail;
			retval = dpram_tty_insert_data(device, (u8 *)(DPRAM_VBASE + (device->in_buff_addr + tail)), size);
			if(size!= retval)
				printk("[OneDRAM: size: %d, retval: %d\n", size, retval);
#ifdef PRINT_READ_SHORT
			else
				printk("READ -return: %d\n", retval);
#endif
		}

		// +++++++ head ------------ tail ++++++++ //
		else {
			int tmp_size = 0;

			// Total Size.
			size = device->in_buff_size - tail + head;

			// 1. tail -> buffer end.
			tmp_size = device->in_buff_size - tail;
			retval = dpram_tty_insert_data(device, (u8 *)(DPRAM_VBASE + (device->in_buff_addr + tail)), tmp_size);
			if(tmp_size!= retval)
				printk("[OneDRAM: size: %d, retval: %d\n", size, retval);
#ifdef PRINT_READ_SHORT
			else
				printk("READ -return: %d\n", retval);
#endif

			// 2. buffer start -> head.
			if (size > tmp_size) {
				retval_add = dpram_tty_insert_data(device, (u8 *)(DPRAM_VBASE + device->in_buff_addr), size - tmp_size);
				retval += retval_add;
		
				if((size - tmp_size)!= retval_add)
					printk("[OneDRAM: size - tmp_size: %d, retval_add: %d\n", size - tmp_size, retval_add);
#ifdef PRINT_READ_SHORT
				else
					printk("READ -return_add: %d\n", retval_add);
#endif


			}
		}

		/* new tail */
		up_tail = (u16)((tail + retval) % device->in_buff_size);
		WRITE_TO_DPRAM_VERIFY(device->in_tail_addr, &up_tail, sizeof(up_tail));
	}
		

	device->in_head_saved = head;
	device->in_tail_saved = tail;

	onedram_release_lock(__func__);
	if (non_cmd & device->mask_req_ack) 	
		send_interrupt_to_phone_with_semaphore(INT_NON_COMMAND(device->mask_res_ack));

	return retval;
	
}

#ifdef _ENABLE_ERROR_DEVICE
void request_phone_reset()
{
	char buf[DPRAM_ERR_MSG_LEN];
	unsigned long flags;

	memset((void *)buf, 0, sizeof (buf));
	buf[0] = '9';
	buf[1] = ' ';
    
	memcpy(buf+2, "$PHONE-RESET", sizeof("$PHONE-RESET"));
	printk("[PHONE ERROR] ->> %s\n", buf);
    
	local_irq_save(flags);
	memcpy(dpram_err_buf, buf, DPRAM_ERR_MSG_LEN);
	dpram_err_len = 64;
	local_irq_restore(flags);
    
	wake_up_interruptible(&dpram_err_wait_q);
	kill_fasync(&dpram_err_async_q, SIGIO, POLL_IN);
}
#endif

static int onedram_get_semaphore(const char *func)
{
	int i, chk_try = 10;
	int j, req_try = 3;

	const u16 cmd = INT_COMMAND(INT_MASK_CMD_SMP_REQ);
	
	if(dump_on) return -1;

	for(j = 0; j < req_try; j++) {
		for(i = 0; i < chk_try; i++) {
			if(*onedram_sem) {
				unreceived_semaphore = 0;
				return 1;
			}
			mdelay(1);
		}
		*onedram_mailboxBA = cmd;
#ifdef PRINT_SEND_IRQ
		printk("=====> send IRQ: %x\n", cmd);
#endif
	}

	unreceived_semaphore++;
	printk("[OneDRAM](%s) Failed to get a Semaphore. sem:%d, PHONE_ACTIVE:%s, fail_cnt:%d\n", 
			func, *onedram_sem,	gpio_get_value(GPIO_PHONE_ACTIVE_AP)?"HIGH":"LOW ", unreceived_semaphore);

#ifdef _ENABLE_ERROR_DEVICE
	if(unreceived_semaphore > 10)
		request_phone_reset();
#endif

	return 0;
}

static void send_interrupt_to_phone_with_semaphore(u16 irq_mask)
{
	if(dump_on) return;

	if(!atomic_read(&onedram_lock)) 
	{
		if(*onedram_sem) { 	
			*onedram_sem = 0x0;
			*onedram_mailboxBA = irq_mask;
#ifdef PRINT_SEND_IRQ
			printk("=====> send IRQ: %x with sem\n", irq_mask);
#endif
			requested_semaphore = 0;
		}else {
			*onedram_mailboxBA = irq_mask;
#ifdef PRINT_SEND_IRQ
			printk("=====> send IRQ: %x\n", irq_mask);
#endif
		}
	}else {
		printk("[OneDRAM] (%s) lock set. can't return semaphore.\n", __func__);
	}


}
static int return_onedram_semaphore(const char* func)
{
	
	if(!atomic_read(&onedram_lock)) 
	{
		if(*onedram_sem) { 	*onedram_sem = 0x0;
			return 1;
		}
	}else {
		requested_semaphore++;
		printk("[OneDRAM] (%s) PDA is accessing onedram. %d\n", __func__, requested_semaphore);
	}

	return 0;

}

static int onedram_lock_with_semaphore(const char* func)
{	
	int lock_value;

	if(!(lock_value = atomic_inc_return(&onedram_lock)))
		printk("[OneDRAM] (%s, lock) fail to locking onedram access. %d\n", func, lock_value);
	
	if(lock_value != 1)
		printk("[OneDRAM] (%s, lock) lock_value: %d\n", func, lock_value);

	if(*onedram_sem) {
		return 0;	
	}
	else{
		printk("[OneDRAM] (%s, lock) failed.. no sem\n", func);
		if((lock_value = atomic_dec_return(&onedram_lock)) < 0)
			printk("[OneDRAM] (%s, lock) fail to unlocking onedram access. %d\n", func, lock_value);

		if(lock_value != 0)
			printk("[OneDRAM] (%s, lock) lock_value: %d\n", func, lock_value);
		return -1;
	}
}

static void onedram_release_lock(const char* func)
{
	int lock_value;

	if((lock_value = atomic_dec_return(&onedram_lock)) < 0)
		printk("[OneDRAM] (%s, release) fail to unlocking onedram access. %d\n", func, lock_value);
		
	if(requested_semaphore) {
		if(!atomic_read(&onedram_lock)) {
			if(*onedram_sem) { 	
				printk("[OneDRAM] (%s, release) requested semaphore(%d) return to Phone.\n", func, requested_semaphore);
				*onedram_sem = 0x0;
				requested_semaphore = 0;
			}
		}
	}

	if(lock_value != 0)
		printk("[OneDRAM] (%s, release) lock_value: %d\n", func, lock_value);

}

static int dpram_shared_bank_remap(void)
{
	dpram_base = ioremap_nocache(DPRAM_START_ADDRESS_PHYS + DPRAM_SHARED_BANK, DPRAM_SHARED_BANK_SIZE);
	if (dpram_base == NULL) {
		printk("failed ioremap\n");
		return -ENOENT;
		}
		
	onedram_sem = DPRAM_VBASE + DPRAM_SMP; 
	onedram_mailboxBA = DPRAM_VBASE + DPRAM_MBX_BA;
	onedram_mailboxAB = DPRAM_VBASE + DPRAM_MBX_AB;
	atomic_set(&onedram_lock, 0);
#if 1
	printk("onedram semaphore value = 0x%x\n", *onedram_sem);
	printk("onedram mailboxAB value = 0x%x\n", *onedram_mailboxAB);
	printk("onedram mailboxBA value = 0x%x\n", *onedram_mailboxBA);
	printk("onedram lock value = %d\n", atomic_read(&onedram_lock));
	atomic_inc(&onedram_lock);
	printk("onedram lock value after inc= %d\n", atomic_read(&onedram_lock));
	atomic_dec(&onedram_lock);
	printk("onedram lock value after dec= %d\n", atomic_read(&onedram_lock));
	printk("onedram lock value inc & test = %d\n", atomic_inc_and_test(&onedram_lock));
	printk("onedram lock value dec & test = %d\n", atomic_dec_and_test(&onedram_lock));
	printk("onedram lock value inc & return = %d\n", atomic_inc_return(&onedram_lock));
	printk("onedram lock value dec & return = %d\n", atomic_dec_return(&onedram_lock));
	printk("[DPRAM] ioremap success. dpram base addr = 0x%08x\n", dpram_base);
#endif


	return 0;
}

static void dpram_clear(void)
{
	long i = 0;
	unsigned long flags;
	
	u16 value = 0;

	/* @LDK@ clear DPRAM except interrupt area */
	local_irq_save(flags);

	for (i = DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS;
			i < DPRAM_SIZE - (DPRAM_INTERRUPT_PORT_SIZE * 2);
			i += 2)
	{
		*((u16 *)(DPRAM_VBASE + i)) = 0;
	}

	local_irq_restore(flags);

	value = *onedram_mailboxAB;
}

static int dpram_init_and_report(void)
{
	const u16 magic_code = 0x00aa;
	const u16 init_start = INT_COMMAND(INT_MASK_CMD_INIT_START);
	const u16 init_end = INT_COMMAND(INT_MASK_CMD_INIT_END);
	u16 ac_code = 0;

	if (!(*onedram_sem)) {
		printk("[OneDRAM] %s, sem: %d\n", __func__, *onedram_sem);
		if(!onedram_get_semaphore(__func__)) {
			printk("[OneDRAM] %s failed to onedram init!!! semaphore: %d\n", __func__, *onedram_sem);
			return -1;
		}
	}

	if(onedram_lock_with_semaphore(__func__) < 0)
		return -1;
#if 0
        /* @LDK@ send init start code to phone */
	*onedram_mailboxBA = init_start;
	printk("[OneDRAM] Send to MailboxBA 0x%x (onedram init start).\n", init_start);
#endif

	/* @LDK@ write DPRAM disable code */
	WRITE_TO_DPRAM(DPRAM_ACCESS_ENABLE_ADDRESS, &ac_code, sizeof(ac_code));

	/* @LDK@ dpram clear */
	dpram_clear();

	/* @LDK@ write magic code */
	WRITE_TO_DPRAM(DPRAM_MAGIC_CODE_ADDRESS, &magic_code, sizeof(magic_code));

	/* @LDK@ write DPRAM enable code */
	ac_code = 0x0001;
	WRITE_TO_DPRAM(DPRAM_ACCESS_ENABLE_ADDRESS, &ac_code, sizeof(ac_code));

	/* @LDK@ send init end code to phone */
	onedram_release_lock(__func__);
	send_interrupt_to_phone_with_semaphore(init_end);
	printk("[OneDRAM] Send 0x%x to MailboxBA  (onedram init finish).\n", init_end);

	phone_sync = 1;
	return 0;
}

static inline int dpram_get_read_available(dpram_device_t *device)
{
	u16 head, tail;
	
	if(*onedram_sem) {

		READ_FROM_DPRAM_VERIFY(&head, device->in_head_addr, sizeof(head));
		READ_FROM_DPRAM_VERIFY(&tail, device->in_tail_addr, sizeof(tail));
		printk("H: %d, T: %d, H-T: %d\n",head, tail, head-tail);

		return head - tail;
	}
	else {
	 	printk("[OneDRAM] (%s) semaphore: %d\n", __func__, *onedram_sem);
		return 0;
	}
}

static void dpram_drop_data(dpram_device_t *device)
{
	u16 head, tail;

	if(*onedram_sem) {
		READ_FROM_DPRAM_VERIFY(&head, device->in_head_addr, sizeof(head));
		WRITE_TO_DPRAM_VERIFY(device->in_tail_addr, &head, sizeof(head));
		
		READ_FROM_DPRAM_VERIFY(&tail, device->in_tail_addr, sizeof(tail));
		printk("[OneDram] %s, head: %d, tail: %d\n", __func__, head, tail);

	}
}

static void dpram_phone_power_on(void)
{

	printk("[OneDRAM] Phone Power on! sem: %d lock: %d\n", *onedram_sem, atomic_read(&onedram_lock));
	*onedram_sem = 0x00;
	printk("[OneDRAM] set semaphore: %d\n", *onedram_sem);

	if(system_rev == 0x0020) {
		printk("[OneDRAM] power control (with GPIO_RESOUT_N_AP)\n");
		gpio_set_value(GPIO_PHONE_ON, GPIO_LEVEL_HIGH);
		mdelay(50);
		gpio_direction_output(GPIO_RESOUT_N_AP, GPIO_LEVEL_LOW);
		mdelay(100);
		gpio_direction_input(GPIO_RESOUT_N_AP);
		mdelay(500);
		gpio_set_value(GPIO_PHONE_ON, GPIO_LEVEL_LOW);
	}
	else {
		printk("[OneDRAM] power control (with GPIO_PHONE_RST_N)\n");
		gpio_set_value(GPIO_PHONE_ON, GPIO_LEVEL_HIGH);
		mdelay(50);
		gpio_set_value(GPIO_PHONE_RST_N, GPIO_LEVEL_LOW);
		mdelay(100);
		gpio_set_value(GPIO_PHONE_RST_N, GPIO_LEVEL_HIGH);
		mdelay(500);
		gpio_set_value(GPIO_PHONE_ON, GPIO_LEVEL_LOW);
	}
}

static void dpram_phone_power_off(void)
{
	printk("[OneDRAM] Phone power Off. - do nothing\n");
}

static int dpram_phone_getstatus(void)
{
	return gpio_get_value(GPIO_PHONE_ACTIVE_AP);
}

static void dpram_phone_reset(void)
{
	printk("[OneDRAM] Phone Reset! sem: %d lock: %d\n", *onedram_sem, atomic_read(&onedram_lock));
	if(*onedram_sem) {
		*onedram_sem = 0x00;
		printk("[OneDRAM] set semaphore: %d\n", *onedram_sem);
	}
	
	if(system_rev == 0x0020) {
		gpio_direction_output(GPIO_RESOUT_N_AP, GPIO_LEVEL_LOW);
		mdelay(100);
		gpio_direction_input(GPIO_RESOUT_N_AP);
	}
	else {
		gpio_set_value(GPIO_PHONE_RST_N, GPIO_LEVEL_LOW);
		mdelay(100);
		gpio_set_value(GPIO_PHONE_RST_N, GPIO_LEVEL_HIGH);
	}

}

static int dpram_extra_mem_rw(struct _param_em *param)
{

	if(param->offset + param->size > 0xFFF800) {
		printk("[OneDRAM] %s failed.. wrong rage of external memory access\n", __func__);
		return -1;
	}

	if(!onedram_get_semaphore(__func__)) 
		return -1;

	if(onedram_lock_with_semaphore(__func__) < 0)
		return -1;

	if (param->rw) {	//write
		WRITE_TO_DPRAM(param->offset, param->addr, param->size);
	}
	else {				//read
		READ_FROM_DPRAM(param->addr, param->offset, param->size);
	}

	onedram_release_lock(__func__);
	return 0;
}

#if 0
static void dpram_mem_rw(struct _mem_param *param)
{
	/* @LDK@ write */
	if (param->dir) {
		WRITE_TO_DPRAM(param->addr, (void *)&param->data, sizeof(param->data));
	}

	/* @LDK@ read */
	else {
		READ_FROM_DPRAM((void *)&param->data, param->addr, sizeof(param->data));
	}
}
#endif

 
static int dpram_phone_ramdump_on(void)
{
	const u16 rdump_flag1 = 0x554C;
	const u16 rdump_flag2 = 0x454D;
//	const u16 temp1, temp2;
	
	printk("[OneDRAM] Ramdump ON.\n");

	if(!onedram_get_semaphore(__func__)) 
		return -1;

	if(onedram_lock_with_semaphore(__func__) < 0)
		return -1;

	WRITE_TO_DPRAM(DPRAM_MAGIC_CODE_ADDRESS,    &rdump_flag1, sizeof(rdump_flag1));
	WRITE_TO_DPRAM(DPRAM_ACCESS_ENABLE_ADDRESS, &rdump_flag2, sizeof(rdump_flag2));

#if 0
	READ_FROM_DPRAM((void *)&temp1, DPRAM_MAGIC_CODE_ADDRESS, sizeof(temp1));
	READ_FROM_DPRAM((void *)&temp2, DPRAM_ACCESS_ENABLE_ADDRESS, sizeof(temp2));
	printk("[OneDRAM] flag1: %x flag2: %x\n", temp1, temp2);
#endif

	/* @LDK@ send init end code to phone */
	onedram_release_lock(__func__);

	//usb_switch_mode(2);

	dump_on = 1;

//	*onedram_sem = 0x00;
	return_onedram_semaphore(__func__);
	if(*onedram_sem) {
		printk("[OneDRAM] Failed to return semaphore. try again\n");
		*onedram_sem = 0x00;
	}

	dpram_phone_reset();
	return 0;

}

static int dpram_phone_ramdump_off(void)
{
	const u16 rdump_flag1 = 0x00aa;
	const u16 rdump_flag2 = 0x0001;
//	const u16 temp1, temp2;

	printk("[OneDRAM] Ramdump OFF.\n");
	
	dump_on = 0;

	if(!onedram_get_semaphore(__func__)) 
		return -1;

	if(onedram_lock_with_semaphore(__func__) < 0)
		return -1;

	WRITE_TO_DPRAM(DPRAM_MAGIC_CODE_ADDRESS,    &rdump_flag1, sizeof(rdump_flag1));
	WRITE_TO_DPRAM(DPRAM_ACCESS_ENABLE_ADDRESS, &rdump_flag2, sizeof(rdump_flag2));
#if 0
	READ_FROM_DPRAM((void *)&temp1, DPRAM_MAGIC_CODE_ADDRESS, sizeof(temp1));
	READ_FROM_DPRAM((void *)&temp2, DPRAM_ACCESS_ENABLE_ADDRESS, sizeof(temp2));
	printk("[OneDRAM] flag1: %x flag2: %x\n", temp1, temp2);
#endif
	/* @LDK@ send init end code to phone */
	onedram_release_lock(__func__);

	//usb_switch_mode(1); 
	
	phone_sync = 0;

//	*onedram_sem = 0x00;
	return_onedram_semaphore(__func__);
	if(*onedram_sem) {
		printk("[OneDRAM] Failed to return semaphore. try again\n");
		*onedram_sem = 0x00;
	}
	dpram_phone_reset();
	return 0;

}

#ifdef CONFIG_PROC_FS
static int dpram_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	char *p = page;
	int len;

	u16 magic, enable;
	u16 fmt_in_head, fmt_in_tail, fmt_out_head, fmt_out_tail;
	u16 raw_in_head, raw_in_tail, raw_out_head, raw_out_tail;
	u16 in_interrupt = 0, out_interrupt = 0;

	int fih, fit, foh, fot;
	int rih, rit, roh, rot;
	int sem;

#ifdef _ENABLE_ERROR_DEVICE
	char buf[DPRAM_ERR_MSG_LEN];
	unsigned long flags;
#endif	/* _ENABLE_ERROR_DEVICE */
	
	if(*onedram_sem) {

		READ_FROM_DPRAM((void *)&magic, DPRAM_MAGIC_CODE_ADDRESS, sizeof(magic));
		READ_FROM_DPRAM((void *)&enable, DPRAM_ACCESS_ENABLE_ADDRESS, sizeof(enable));
		READ_FROM_DPRAM((void *)&fmt_in_head, DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS, sizeof(fmt_in_head));
		READ_FROM_DPRAM((void *)&fmt_in_tail, DPRAM_PHONE2PDA_FORMATTED_TAIL_ADDRESS, sizeof(fmt_in_tail));
		READ_FROM_DPRAM((void *)&fmt_out_head, DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS, sizeof(fmt_out_head));
		READ_FROM_DPRAM((void *)&fmt_out_tail, DPRAM_PDA2PHONE_FORMATTED_TAIL_ADDRESS, sizeof(fmt_out_tail));
		READ_FROM_DPRAM((void *)&raw_in_head, DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS, sizeof(raw_in_head));
		READ_FROM_DPRAM((void *)&raw_in_tail, DPRAM_PHONE2PDA_RAW_TAIL_ADDRESS, sizeof(raw_in_tail));
		READ_FROM_DPRAM((void *)&raw_out_head, DPRAM_PDA2PHONE_RAW_HEAD_ADDRESS, sizeof(raw_out_head));
		READ_FROM_DPRAM((void *)&raw_out_tail, DPRAM_PDA2PHONE_RAW_TAIL_ADDRESS, sizeof(raw_out_tail));
	}
	else {
		magic = enable = 0;
		fmt_in_head = fmt_in_tail = fmt_out_head = fmt_out_tail = 0;
		raw_in_head = raw_in_tail = raw_out_head = raw_out_tail = 0;
	}

	fih = dpram_table[FORMATTED_INDEX].in_head_saved;
	fit = dpram_table[FORMATTED_INDEX].in_tail_saved;
	foh = dpram_table[FORMATTED_INDEX].out_head_saved;
	fot = dpram_table[FORMATTED_INDEX].out_tail_saved;
	rih = dpram_table[RAW_INDEX].in_head_saved;
	rit = dpram_table[RAW_INDEX].in_tail_saved;
	roh = dpram_table[RAW_INDEX].out_head_saved;
	rot = dpram_table[RAW_INDEX].out_tail_saved;

	sem = *onedram_sem;

	in_interrupt = *onedram_mailboxAB;
	out_interrupt = *onedram_mailboxBA;

#ifdef _ENABLE_ERROR_DEVICE
	memset((void *)buf, '\0', DPRAM_ERR_MSG_LEN);
	local_irq_save(flags);
	memcpy(buf, dpram_err_buf, DPRAM_ERR_MSG_LEN - 1);
	local_irq_restore(flags);
#endif	/* _ENABLE_ERROR_DEVICE */

	p += sprintf(p,
			"-------------------------------------\n"
			"| NAME\t\t\t| VALUE\n"
			"-------------------------------------\n"
			"|R MAGIC CODE\t\t| 0x%04x\n"
			"|R ENABLE CODE\t\t| 0x%04x\n"
			"|R PHONE->PDA FMT HEAD\t| %u\n"
			"|R PHONE->PDA FMT TAIL\t| %u\n"
			"|R PDA->PHONE FMT HEAD\t| %u\n"
			"|R PDA->PHONE FMT TAIL\t| %u\n"
			"|R PHONE->PDA RAW HEAD\t| %u\n"
			"|R RPHONE->PDA RAW TAIL\t| %u\n"
			"|R PDA->PHONE RAW HEAD\t| %u\n"
			"|R PDA->PHONE RAW TAIL\t| %u\n"
			"-------------------------------------\n"
			"| Onedram Semaphore\t| %d\n"
			"| requested Semaphore\t| %d\n"
			"| unreceived Semaphore\t| %d\n"
			"-------------------------------------\n"
			"| FMT PHONE->PDA HEAD\t| %d\n"
			"| FMT PHONE->PDA TAIL\t| %d\n"
			"| FMT PDA->PHONE HEAD\t| %d\n"
			"| FMT PDA->PHONE TAIL\t| %d\n"
			"-------------------------------------\n"
			"| RAW PHONE->PDA HEAD\t| %d\n"
			"| RAW PHONE->PDA TAIL\t| %d\n"
			"| RAW PDA->PHONE HEAD\t| %d\n"
			"| RAW PDA->PHONE TAIL\t| %d\n"
			"-------------------------------------\n"
			"| PHONE->PDA MAILBOX\t| 0x%04x\n"
			"| PDA->PHONE MAILBOX\t| 0x%04x\n"
			"-------------------------------------\n"
#ifdef _ENABLE_ERROR_DEVICE
			"| LAST PHONE ERR MSG\t| %s\n"
#endif	/* _ENABLE_ERROR_DEVICE */
			"| PHONE ACTIVE\t\t| %s\n"
			"| DPRAM INT Level\t| %d\n"
			"-------------------------------------\n",
			magic, enable,
			fmt_in_head, fmt_in_tail, fmt_out_head, fmt_out_tail,
			raw_in_head, raw_in_tail, raw_out_head, raw_out_tail,
			sem, 
			requested_semaphore,
			unreceived_semaphore,
			fih, fit, foh, fot, 
			rih, rit, roh, rot,
			in_interrupt, out_interrupt,

#ifdef _ENABLE_ERROR_DEVICE
			(buf[0] != '\0' ? buf : "NONE"),
#endif	/* _ENABLE_ERROR_DEVICE */

			(dpram_phone_getstatus() ? "ACTIVE" : "INACTIVE"),
				gpio_get_value(IRQ_PHONE_ACTIVE)
		);

	len = (p - page) - off;
	if (len < 0) {
		len = 0;
	}

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}
#endif /* CONFIG_PROC_FS */

/* dpram tty file operations. */
static int dpram_tty_open(struct tty_struct *tty, struct file *file)
{
	dpram_device_t *device = &dpram_table[tty->index];

	device->serial.tty = tty;
	device->serial.open_count++;

	if (device->serial.open_count > 1) {
		device->serial.open_count--;
		return -EBUSY;
	}

	tty->driver_data = (void *)device;
	tty->low_latency = 1;
	return 0;
}

static void dpram_tty_close(struct tty_struct *tty, struct file *file)
{
	dpram_device_t *device = (dpram_device_t *)tty->driver_data;

	if (device && (device == &dpram_table[tty->index])) {
		down(&device->serial.sem);
		device->serial.open_count--;
		device->serial.tty = NULL;
		up(&device->serial.sem);
	}
}

static int dpram_tty_write(struct tty_struct *tty,
		const unsigned char *buffer, int count)
{
	dpram_device_t *device = (dpram_device_t *)tty->driver_data;

	if (!device) {
		return 0;
	}

	return dpram_write(device, buffer, count);
}

static int dpram_tty_write_room(struct tty_struct *tty)
{
	int avail;
	u16 head, tail;

	dpram_device_t *device = (dpram_device_t *)tty->driver_data;

	if (device != NULL) {
#if 0
		onedram_lock_with_semaphore();

		READ_FROM_DPRAM_VERIFY(&head, device->out_head_addr, sizeof(head));
		READ_FROM_DPRAM_VERIFY(&tail, device->out_tail_addr, sizeof(tail));

		onedram_release_lock();
#else
		head = device->out_head_saved;
		tail = device->out_tail_saved;
#endif
		avail = (head < tail) ? tail - head - 1 :
			device->out_buff_size + tail - head - 1;

		return avail;
	}

	return 0;
}


static int dpram_tty_ioctl(struct tty_struct *tty, struct file *file,
		unsigned int cmd, unsigned long arg)
{
	unsigned int val;

	switch (cmd) {
		case DPRAM_PHONE_ON:
			phone_sync = 0;
			dump_on = 0;
			requested_semaphore = 0;
			unreceived_semaphore = 0;
			dpram_phone_power_on();
#if 0
			if (!(*onedram_sem)) {
				printk("[OneDRAM] (%s) semaphore: %d\n", __func__, *onedram_sem);
				onedram_get_semaphore(__func__);
			}
#endif
			return 0;

		case DPRAM_PHONE_GETSTATUS:
			val = dpram_phone_getstatus();
			return copy_to_user((unsigned int *)arg, &val, sizeof(val));

		case DPRAM_PHONE_RESET:
			phone_sync = 0;
			requested_semaphore = 0;
			unreceived_semaphore = 0;
			dpram_phone_reset();
			return 0;

		case DPRAM_PHONE_OFF:
			dpram_phone_power_off();
			return 0;

		case DPRAM_PHONE_RAMDUMP_ON:
			dpram_phone_ramdump_on();
			return 0;

		case DPRAM_PHONE_RAMDUMP_OFF:
			dpram_phone_ramdump_off();
			return 0;

		case DPRAM_EXTRA_MEM_RW:
		{
			struct _param_em param;

			val = copy_from_user((void *)&param, (void *)arg, sizeof(param));
			if (dpram_extra_mem_rw(&param) < 0) {
				printk("[OneDRAM] external memory access fail..\n");
				return -1;
			}
			if (!param.rw) {	//read
				return copy_to_user((unsigned long *)arg, &param, sizeof(param));
			}

			return 0;
		}

#if 0
		case DPRAM_MEM_RW:
		{
			struct _mem_param param;

			val = copy_from_user((void *)&param, (void *)arg, sizeof(param));
			dpram_mem_rw(&param);

			if (!param.dir) {
				return copy_to_user((unsigned long *)arg, &param, sizeof(param));
			}

			return 0;
		}
#endif
		default:
			break;
	}

	return -ENOIOCTLCMD;
}

static int dpram_tty_chars_in_buffer(struct tty_struct *tty)
{
	int data;
	u16 head, tail;

	dpram_device_t *device = (dpram_device_t *)tty->driver_data;

	if (device != NULL) {
#if 0
		onedram_lock_with_semaphore();

		READ_FROM_DPRAM_VERIFY(&head, device->out_head_addr, sizeof(head));
		READ_FROM_DPRAM_VERIFY(&tail, device->out_tail_addr, sizeof(tail));

		onedram_release_lock();
#else
		head = device->out_head_saved;
		tail = device->out_tail_saved;
#endif
		data = (head > tail) ? head - tail - 1 :
			device->out_buff_size - tail + head;

		return data;
	}

	return 0;
}

#ifdef _ENABLE_ERROR_DEVICE
static int dpram_err_read(struct file *filp, char *buf, size_t count, loff_t *ppos)
{
	DECLARE_WAITQUEUE(wait, current);

	unsigned long flags;
	ssize_t ret;
	size_t ncopy;

	add_wait_queue(&dpram_err_wait_q, &wait);
	set_current_state(TASK_INTERRUPTIBLE);

	while (1) {
		local_irq_save(flags);

		if (dpram_err_len) {
			ncopy = min(count, dpram_err_len);

			if (copy_to_user(buf, dpram_err_buf, ncopy)) {
				ret = -EFAULT;
			}

			else {
				ret = ncopy;
			}

			dpram_err_len = 0;
			
			local_irq_restore(flags);
			break;
		}

		local_irq_restore(flags);

		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			break;
		}

		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			break;
		}

		schedule();
	}

	set_current_state(TASK_RUNNING);
	remove_wait_queue(&dpram_err_wait_q, &wait);

	return ret;
}

static int dpram_err_fasync(int fd, struct file *filp, int mode)
{
	return fasync_helper(fd, filp, mode, &dpram_err_async_q);
}

static unsigned int dpram_err_poll(struct file *filp,
		struct poll_table_struct *wait)
{
	poll_wait(filp, &dpram_err_wait_q, wait);
	return ((dpram_err_len) ? (POLLIN | POLLRDNORM) : 0);
}
#endif	/* _ENABLE_ERROR_DEVICE */

/* handlers. */
static void res_ack_tasklet_handler(unsigned long data)
{
	dpram_device_t *device = (dpram_device_t *)data;

	if (device && device->serial.tty) {
		struct tty_struct *tty = device->serial.tty;

		if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
				tty->ldisc->ops->write_wakeup) {
			(tty->ldisc->ops->write_wakeup)(tty);
		}

		wake_up_interruptible(&tty->write_wait);
	}
}

static void fmt_rcv_tasklet_handler(unsigned long data)
{
	dpram_tasklet_data_t *tasklet_data = (dpram_tasklet_data_t *)data;

	dpram_device_t *device = tasklet_data->device;
	u16 non_cmd = tasklet_data->non_cmd;

	int ret = 0;
	int cnt = 0;

	if (device && device->serial.tty) {
		struct tty_struct *tty = device->serial.tty;

		while (dpram_get_read_available(device)) {
			ret = dpram_read_fmt(device, non_cmd);

            if (!ret) cnt++;

			if (cnt > 10) {
				dpram_drop_data(device);
				break;
			}
			if (ret < 0) {
				printk("%s, dpram_read_fmt failed\n", __func__);
				/* TODO: ... wrong.. */
			}
				tty_flip_buffer_push(tty);
			}
	}

	else {
		dpram_drop_data(device);
	}
}

static void cmd_req_active_handler(void)
{
#if 0
	send_interrupt_to_phone(INT_COMMAND(INT_MASK_CMD_RES_ACTIVE));
#else
	send_interrupt_to_phone_with_semaphore(INT_COMMAND(INT_MASK_CMD_RES_ACTIVE));
#endif
}

static void cmd_error_display_handler(void)
{

#ifdef _ENABLE_ERROR_DEVICE
	char buf[DPRAM_ERR_MSG_LEN];
	unsigned long flags;

	if(*onedram_sem) {
	memset((void *)buf, 0, sizeof (buf));
	buf[0] = '1';
	buf[1] = ' ';
	
	READ_FROM_DPRAM((buf + 2), DPRAM_PHONE2PDA_FORMATTED_BUFFER_ADDRESS,
				sizeof (buf) - 3);


	printk("[PHONE ERROR] ->> %s\n", buf);

	local_irq_save(flags);
	memcpy(dpram_err_buf, buf, DPRAM_ERR_MSG_LEN);
	dpram_err_len = 64;
	local_irq_restore(flags);

	wake_up_interruptible(&dpram_err_wait_q);
	kill_fasync(&dpram_err_async_q, SIGIO, POLL_IN);
	}
#endif	/* _ENABLE_ERROR_DEVICE */

}

static void cmd_phone_start_handler(void)
{


	printk("[OneDRAM] Received 0xc8 from MailboxAB (Phone Boot OK).\n");
	if(!phone_sync) {
		dpram_init_and_report();
	}
}

static void cmd_req_time_sync_handler(void)
{
	/* TODO: add your codes here.. */
}

static void cmd_phone_deep_sleep_handler(void)
{
	/* TODO: add your codes here.. */
}

static void cmd_nv_rebuilding_handler(void)
{
	/* TODO: add your codes here.. */
}

static void cmd_emer_down_handler(void)
{
	/* TODO: add your codes here.. */
}

#if 0
static void cmd_smp_req_handler(void)
{
	const u16 cmd = INT_COMMAND(INT_MASK_CMD_SMP_REP);


	if(return_onedram_semaphore(__func__)) {
		*onedram_mailboxBA = cmd;
#ifdef PRINT_SEND_IRQ
		printk("=====> send IRQ: %x\n", cmd);
#endif
	}
}
#endif

static void cmd_smp_rep_handler(void)
{
	/* TODO: add your codes here.. */
	unreceived_semaphore = 0;
}

static void semaphore_control_handler(unsigned long data)
{
	const u16 cmd = INT_COMMAND(INT_MASK_CMD_SMP_REP);


	if(return_onedram_semaphore(__func__)) {
		*onedram_mailboxBA = cmd;
#ifdef PRINT_SEND_IRQ
		printk("=====> send IRQ: %x\n", cmd);
#endif
	}
}


static void command_handler(u16 cmd)
{
	switch (cmd) {
		case INT_MASK_CMD_REQ_ACTIVE:
			cmd_req_active_handler();
			break;

		case INT_MASK_CMD_ERR_DISPLAY:
			cmd_error_display_handler();
			break;

		case INT_MASK_CMD_PHONE_START:
			cmd_phone_start_handler();
			break;

		case INT_MASK_CMD_REQ_TIME_SYNC:
			cmd_req_time_sync_handler();
			break;

		case INT_MASK_CMD_PHONE_DEEP_SLEEP:
			cmd_phone_deep_sleep_handler();
			break;

		case INT_MASK_CMD_NV_REBUILDING:
			cmd_nv_rebuilding_handler();
			break;

		case INT_MASK_CMD_EMER_DOWN:
			cmd_emer_down_handler();
			break;

		case INT_MASK_CMD_SMP_REQ:
			tasklet_schedule(&semaphore_control_tasklet);
//			cmd_smp_req_handler();
			break;

		case INT_MASK_CMD_SMP_REP:
			cmd_smp_rep_handler();
			break;

		default:
			dprintk("Unknown command.. %x\n", cmd);
	}
}

static void non_command_handler(u16 non_cmd)
{
	u16 head, tail;

	/* @LDK@ formatted check. */


	if(!(*onedram_sem)) {
//		printk("[OneDRAM] %s failed! no sem. cmd: %x\n", __func__, non_cmd);
		return;
	}

	READ_FROM_DPRAM_VERIFY(&head, DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS, sizeof(head));
	READ_FROM_DPRAM_VERIFY(&tail, DPRAM_PHONE2PDA_FORMATTED_TAIL_ADDRESS, sizeof(tail));

	if (head != tail) {
		non_cmd |= INT_MASK_SEND_F;
	}else {
		if(non_cmd & INT_MASK_REQ_ACK_F)
			printk("=====> FMT DATA EMPTY & REQ_ACK_F\n");
	}
	
	/* @LDK@ raw check. */
	READ_FROM_DPRAM_VERIFY(&head, DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS, sizeof(head));
	READ_FROM_DPRAM_VERIFY(&tail, DPRAM_PHONE2PDA_RAW_TAIL_ADDRESS, sizeof(tail));

	if (head != tail) {
		non_cmd |= INT_MASK_SEND_R;
	}else {
		if(non_cmd & INT_MASK_REQ_ACK_R)
			printk("=====> RAW DATA EMPTY & REQ_ACK_R\n");
	}

	/* @LDK@ +++ scheduling.. +++ */
	if (non_cmd & INT_MASK_SEND_F) {
		dpram_tasklet_data[FORMATTED_INDEX].device = &dpram_table[FORMATTED_INDEX];
		dpram_tasklet_data[FORMATTED_INDEX].non_cmd = non_cmd;
		fmt_send_tasklet.data = (unsigned long)&dpram_tasklet_data[FORMATTED_INDEX];
		tasklet_schedule(&fmt_send_tasklet);
	}
	if (non_cmd & INT_MASK_SEND_R) {
		dpram_tasklet_data[RAW_INDEX].device = &dpram_table[RAW_INDEX];
		dpram_tasklet_data[RAW_INDEX].non_cmd = non_cmd;
		raw_send_tasklet.data = (unsigned long)&dpram_tasklet_data[RAW_INDEX];
		/* @LDK@ raw buffer op. -> soft irq level. */
		tasklet_hi_schedule(&raw_send_tasklet);
	}

	if (non_cmd & INT_MASK_RES_ACK_F) {
		tasklet_schedule(&fmt_res_ack_tasklet);
	}

	if (non_cmd & INT_MASK_RES_ACK_R) {
		tasklet_hi_schedule(&raw_res_ack_tasklet);
	}

}

static inline
void check_int_pin_level(void)
{
	u16 mask = 0, cnt = 0;

	while (cnt++ < 3) {
		mask = *onedram_mailboxAB;
		if (gpio_get_value(GPIO_nONED_INT_AP))
			break;
	}
}

/* @LDK@ interrupt handlers. */
static irqreturn_t dpram_irq_handler(int irq, void *dev_id)
{
	u16 irq_mask = 0;
#ifdef PRINT_HEAD_TAIL	
	u16 fih, fit, foh, fot;
	u16 rih, rit, roh, rot;
#endif

	irq_mask = *onedram_mailboxAB;
//	check_int_pin_level();

#ifdef PRINT_RECV_IRQ	
	printk("=====> received IRQ: %x\n", irq_mask);
#endif

#ifdef PRINT_HEAD_TAIL	
	if(*onedram_sem) {
		READ_FROM_DPRAM_VERIFY(&fih, DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS, sizeof(fih));
		READ_FROM_DPRAM_VERIFY(&fit, DPRAM_PHONE2PDA_FORMATTED_TAIL_ADDRESS, sizeof(fit));
		READ_FROM_DPRAM_VERIFY(&foh, DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS, sizeof(foh));
		READ_FROM_DPRAM_VERIFY(&fot, DPRAM_PDA2PHONE_FORMATTED_TAIL_ADDRESS, sizeof(fot));
		READ_FROM_DPRAM_VERIFY(&rih, DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS, sizeof(rih));
		READ_FROM_DPRAM_VERIFY(&rit, DPRAM_PHONE2PDA_RAW_TAIL_ADDRESS, sizeof(rit));
		READ_FROM_DPRAM_VERIFY(&roh, DPRAM_PDA2PHONE_RAW_HEAD_ADDRESS, sizeof(roh));
		READ_FROM_DPRAM_VERIFY(&rot, DPRAM_PDA2PHONE_RAW_TAIL_ADDRESS, sizeof(rot));

		printk("\n fmt_in  H:%4d, T:%4d, M:%4d\n fmt_out H:%4d, T:%4d, M:%4d\n raw_in  H:%4d, T:%4d, M:%4d\n raw out H:%4d, T:%4d, M:%4d\n", fih, fit, DPRAM_PHONE2PDA_FORMATTED_BUFFER_SIZE, foh, fot,DPRAM_PDA2PHONE_FORMATTED_BUFFER_SIZE, rih, rit, DPRAM_PHONE2PDA_RAW_BUFFER_SIZE, roh, rot, DPRAM_PDA2PHONE_RAW_BUFFER_SIZE);
	}
#endif

	/* valid bit verification. @LDK@ */
	if (!(irq_mask & INT_MASK_VALID)) {
		printk("Invalid interrupt mask: 0x%04x\n", irq_mask);
		return IRQ_NONE;
	}

	/* command or non-command? @LDK@ */
	if (irq_mask & INT_MASK_COMMAND) {
		irq_mask &= ~(INT_MASK_VALID | INT_MASK_COMMAND);
		command_handler(irq_mask);
	}
	else {
		irq_mask &= ~INT_MASK_VALID;
		non_command_handler(irq_mask);
	}

	return IRQ_HANDLED;
}

static irqreturn_t phone_active_irq_handler(int irq, void *dev_id)
{
	printk("[OneDRAM] PHONE_ACTIVE level: %s, sem: %d, phone_sync: %d\n", 
			gpio_get_value(GPIO_PHONE_ACTIVE_AP)?"HIGH":"LOW ", *onedram_sem, phone_sync);

#ifdef _ENABLE_ERROR_DEVICE
	if((phone_sync) && (!gpio_get_value(GPIO_PHONE_ACTIVE_AP)))
		request_phone_reset();	
#endif

	return IRQ_HANDLED;
}

/* basic functions. */
#ifdef _ENABLE_ERROR_DEVICE
static struct file_operations dpram_err_ops = {
	.owner = THIS_MODULE,
	.read = dpram_err_read,
	.fasync = dpram_err_fasync,
	.poll = dpram_err_poll,
	.llseek = no_llseek,

	/* TODO: add more operations */
};
#endif	/* _ENABLE_ERROR_DEVICE */

static struct tty_operations dpram_tty_ops = {
	.open 		= dpram_tty_open,
	.close 		= dpram_tty_close,
	.write 		= dpram_tty_write,
	.write_room = dpram_tty_write_room,
	.ioctl 		= dpram_tty_ioctl,
	.chars_in_buffer = dpram_tty_chars_in_buffer,

	/* TODO: add more operations */
};

#ifdef _ENABLE_ERROR_DEVICE

static void unregister_dpram_err_device(void)
{
	unregister_chrdev(DRIVER_MAJOR_NUM, DPRAM_ERR_DEVICE);
	class_destroy(dpram_class);
}

static int register_dpram_err_device(void)
{
	/* @LDK@ 1 = formatted, 2 = raw, so error device is '0' */
	struct device *dpram_err_dev_t;
	int ret = register_chrdev(DRIVER_MAJOR_NUM, DPRAM_ERR_DEVICE, &dpram_err_ops);

	if ( ret < 0 )
	{
		return ret;
	}

	dpram_class = class_create(THIS_MODULE, "err");

	if (IS_ERR(dpram_class))
	{
		unregister_dpram_err_device();
		return -EFAULT;
	}

	dpram_err_dev_t = device_create(dpram_class, NULL,
			MKDEV(DRIVER_MAJOR_NUM, 0), NULL, DPRAM_ERR_DEVICE);

	if (IS_ERR(dpram_err_dev_t))
	{
		unregister_dpram_err_device();
		return -EFAULT;
	}

	return 0;
}
#endif	/* _ENABLE_ERROR_DEVICE */

static int register_dpram_driver(void)
{
	int retval = 0;

	/* @LDK@ allocate tty driver */
	dpram_tty_driver = alloc_tty_driver(MAX_INDEX);

	if (!dpram_tty_driver) {
		return -ENOMEM;
	}

	/* @LDK@ initialize tty driver */
	dpram_tty_driver->owner = THIS_MODULE;
	dpram_tty_driver->magic = TTY_DRIVER_MAGIC;
	dpram_tty_driver->driver_name = DRIVER_NAME;
	dpram_tty_driver->name = "dpram";
	dpram_tty_driver->major = DRIVER_MAJOR_NUM;
	dpram_tty_driver->minor_start = 1;
	dpram_tty_driver->num = 2;
	dpram_tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
	dpram_tty_driver->subtype = SERIAL_TYPE_NORMAL;
	dpram_tty_driver->flags = TTY_DRIVER_REAL_RAW;
	dpram_tty_driver->init_termios = tty_std_termios;
	dpram_tty_driver->init_termios.c_cflag =
		(B115200 | CS8 | CREAD | CLOCAL | HUPCL);

	tty_set_operations(dpram_tty_driver, &dpram_tty_ops);

	dpram_tty_driver->ttys = dpram_tty;
	dpram_tty_driver->termios = dpram_termios;
	dpram_tty_driver->termios_locked = dpram_termios_locked;

	/* @LDK@ register tty driver */
	retval = tty_register_driver(dpram_tty_driver);

	if (retval) {
		dprintk("tty_register_driver error\n");
		put_tty_driver(dpram_tty_driver);
		return retval;
	}

	return 0;
}

static void unregister_dpram_driver(void)
{
	tty_unregister_driver(dpram_tty_driver);
}


static void init_devices(void)
{
	int i;

	for (i = 0; i < MAX_INDEX; i++) {
		init_MUTEX(&dpram_table[i].serial.sem);

		dpram_table[i].serial.open_count = 0;
		dpram_table[i].serial.tty = NULL;
	}
}

static void init_hw_setting(void)
{
//	u32 mask;

	/* initial pin settings - dpram driver control */
	s3c_gpio_cfgpin(GPIO_PHONE_ACTIVE_AP, S3C_GPIO_SFN(GPIO_PHONE_ACTIVE_AP_AF));
	s3c_gpio_setpull(GPIO_PHONE_ACTIVE_AP, S3C_GPIO_PULL_NONE); 
	set_irq_type(IRQ_PHONE_ACTIVE, IRQ_TYPE_EDGE_BOTH);

	s3c_gpio_cfgpin(GPIO_nONED_INT_AP, S3C_GPIO_SFN(GPIO_nONED_INT_AP_AF));
	s3c_gpio_setpull(GPIO_nONED_INT_AP, S3C_GPIO_PULL_NONE); 
	set_irq_type(IRQ_ONEDRAM_INT_N, IRQ_TYPE_EDGE_FALLING);

	if (gpio_is_valid(GPIO_PHONE_ON)) {
		if (gpio_request(GPIO_PHONE_ON, S3C_GPIO_LAVEL(GPIO_PHONE_ON)))
			printk(KERN_ERR "Filed to request GPIO_PHONE_ON!\n");
		gpio_direction_output(GPIO_PHONE_ON, GPIO_LEVEL_LOW);
	}
	s3c_gpio_setpull(GPIO_PHONE_ON, S3C_GPIO_PULL_UP); 
	gpio_set_value(GPIO_PHONE_ON, GPIO_LEVEL_LOW);

printk("Dpram: system_rev: %x\n", system_rev);
	if(system_rev == 0x0020) {
		if (gpio_is_valid(GPIO_RESOUT_N_AP)) {
			if (gpio_request(GPIO_RESOUT_N_AP, S3C_GPIO_LAVEL(GPIO_RESOUT_N_AP)))
				printk(KERN_ERR "Filed to request GPIO_RESOUT_N_AP!\n");
			gpio_direction_input(GPIO_RESOUT_N_AP);
		}
	}
	else {
		if (gpio_is_valid(GPIO_PHONE_RST_N)) {
			if (gpio_request(GPIO_PHONE_RST_N, S3C_GPIO_LAVEL(GPIO_PHONE_RST_N)))
				printk(KERN_ERR "Filed to request GPIO_PHONE_RST_N!\n");
			gpio_direction_output(GPIO_PHONE_RST_N, GPIO_LEVEL_HIGH);
		}
		s3c_gpio_setpull(GPIO_PHONE_RST_N, S3C_GPIO_PULL_UP); 
	}

	if (gpio_is_valid(GPIO_PDA_ACTIVE)) {
		if (gpio_request(GPIO_PDA_ACTIVE, S3C_GPIO_LAVEL(GPIO_PDA_ACTIVE)))
			printk(KERN_ERR "Filed to request GPIO_PDA_ACTIVE!\n");
		gpio_direction_output(GPIO_PDA_ACTIVE, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_PDA_ACTIVE, S3C_GPIO_PULL_NONE); 
	
}

static void kill_tasklets(void)
{
	tasklet_kill(&fmt_res_ack_tasklet);
	tasklet_kill(&raw_res_ack_tasklet);

	tasklet_kill(&fmt_send_tasklet);
	tasklet_kill(&raw_send_tasklet);
}

static int register_interrupt_handler(void)
{

	unsigned int dpram_irq, phone_active_irq;
	int retval = 0;
	
	dpram_irq = IRQ_ONEDRAM_INT_N;
	phone_active_irq = IRQ_PHONE_ACTIVE;

	/* @LDK@ interrupt area read - pin level will be driven high. */
//	dpram_clear();

	/* @LDK@ dpram interrupt */
	retval = request_irq(dpram_irq, dpram_irq_handler, IRQF_DISABLED, "dpram irq", NULL);

	if (retval) {
		dprintk("DPRAM interrupt handler failed.\n");
		unregister_dpram_driver();
		return -1;
	}

	/* @LDK@ phone active interrupt */
	retval = request_irq(phone_active_irq, phone_active_irq_handler, IRQF_DISABLED, "Phone Active", NULL);

	if (retval) {
		dprintk("Phone active interrupt handler failed.\n");
		free_irq(phone_active_irq, NULL);
		unregister_dpram_driver();
		return -1;
	}

	return 0;
}

static void check_miss_interrupt(void)
{
	unsigned long flags;

	if (gpio_get_value(GPIO_PHONE_ACTIVE_AP) &&
			(!gpio_get_value(GPIO_nONED_INT_AP))) {
		dprintk("there is a missed interrupt. try to read it!\n");

		if (!(*onedram_sem)) {
			printk("[OneDRAM] (%s) semaphore: %d\n", __func__, *onedram_sem);
			onedram_get_semaphore(__func__);
		}

		local_irq_save(flags);
		dpram_irq_handler(IRQ_ONEDRAM_INT_N, NULL);
		local_irq_restore(flags);
	}
}

static int dpram_suspend(struct platform_device *dev, pm_message_t state)
{
	gpio_set_value(GPIO_PDA_ACTIVE, GPIO_LEVEL_LOW);
	if(requested_semaphore)
		printk("=====> %s requested semaphore: %d\n", __func__, requested_semaphore);
	return 0;
}

static int dpram_resume(struct platform_device *dev)
{
	gpio_set_value(GPIO_PDA_ACTIVE, GPIO_LEVEL_HIGH);
	if(requested_semaphore)
		printk("=====> %s requested semaphore: %d\n", __func__, requested_semaphore);
	check_miss_interrupt();
	return 0;
}

static int __devinit dpram_probe(struct platform_device *dev)
{
	int retval;
//	system_rev = 0x20; //bss

	/* @LDK@ register dpram (tty) driver */
	retval = register_dpram_driver();
	if (retval) {
		dprintk("Failed to register dpram (tty) driver.\n");
		return -1;
	}

#ifdef _ENABLE_ERROR_DEVICE
	/* @LDK@ register dpram error device */
	retval = register_dpram_err_device();
	if (retval) {
		dprintk("Failed to register dpram error device.\n");

		unregister_dpram_driver();
		return -1;
	}

	memset((void *)dpram_err_buf, '\0', sizeof dpram_err_buf);
#endif /* _ENABLE_ERROR_DEVICE */

	/* @LDK@ H/W setting */
	init_hw_setting();

	dpram_shared_bank_remap();

	/* @LDK@ initialize device table */
	init_devices();

	/* @LDK@ register interrupt handler */
	if ((retval = register_interrupt_handler()) < 0) {
		return -1;
	}
#ifdef CONFIG_PROC_FS
	create_proc_read_entry(DRIVER_PROC_ENTRY, 0, 0, dpram_read_proc, NULL);
#endif	/* CONFIG_PROC_FS */

	/* @LDK@ check out missing interrupt from the phone */
	check_miss_interrupt();
	
	printk("DPRAM probe ok\n");
	return 0;
}

static int __devexit dpram_remove(struct platform_device *dev)
{
	/* @LDK@ unregister dpram (tty) driver */
	unregister_dpram_driver();

	/* @LDK@ unregister dpram error device */
#ifdef _ENABLE_ERROR_DEVICE
	unregister_dpram_err_device();
#endif

	/* @LDK@ unregister irq handler */
	free_irq(IRQ_ONEDRAM_INT_N, NULL);
	free_irq(IRQ_PHONE_ACTIVE, NULL);

	kill_tasklets();

	return 0;
}

static struct platform_driver platform_dpram_driver = {
	.probe		= dpram_probe,
	.remove		= __devexit_p(dpram_remove),
	.suspend	= dpram_suspend,
	.resume 	= dpram_resume,
	.driver	= {
		.name	= "dpram-device",
	},
};

/* init & cleanup. */
static int __init dpram_init(void)
{
	return platform_driver_register(&platform_dpram_driver);
}

static void __exit dpram_exit(void)
{
	platform_driver_unregister(&platform_dpram_driver);
}

module_init(dpram_init);
module_exit(dpram_exit);

MODULE_AUTHOR("SAMSUNG ELECTRONICS CO., LTD");
MODULE_DESCRIPTION("Onedram Device Driver.");
MODULE_LICENSE("GPL");
