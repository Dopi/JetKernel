/*
 *  linux/kernel/panic.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 * This function is used through-out the kernel (including mm and fs)
 * to indicate a major problem.
 */
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/notifier.h>
#include <linux/init.h>
#include <linux/sysrq.h>
#include <linux/interrupt.h>
#include <linux/nmi.h>
#include <linux/kexec.h>
#include <linux/debug_locks.h>
#include <linux/random.h>
#include <linux/kallsyms.h>

#ifdef CONFIG_KERNEL_PANIC_DUMP 
#include <linux/kernel.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/time.h>
 
extern char *PANIC_Base;
extern char *PANIC_Current;

static int multiple_panic = 0;
#endif

int panic_on_oops;
int tainted;
static int pause_on_oops;
static int pause_on_oops_flag;
static DEFINE_SPINLOCK(pause_on_oops_lock);

#ifndef CONFIG_PANIC_TIMEOUT
#define CONFIG_PANIC_TIMEOUT 0
#endif
int panic_timeout = CONFIG_PANIC_TIMEOUT;

ATOMIC_NOTIFIER_HEAD(panic_notifier_list);

EXPORT_SYMBOL(panic_notifier_list);

static int __init panic_setup(char *str)
{
	panic_timeout = simple_strtoul(str, NULL, 0);
	return 1;
}
__setup("panic=", panic_setup);

static long no_blink(long time)
{
	return 0;
}

/* Returns how long it waited in ms */
long (*panic_blink)(long time);
EXPORT_SYMBOL(panic_blink);

/**
 *	panic - halt the system
 *	@fmt: The text string to print
 *
 *	Display a message, then perform cleanups.
 *
 *	This function never returns.
 */

#define REBOOT_WHEN_PANIC_IN_START_KERNEL

extern int start_kernel_ok;

extern void arch_reset(char mode);

NORET_TYPE void panic(const char * fmt, ...)
{
	long i;
	static char buf[1024];
	va_list args;
#if defined(CONFIG_S390)
	unsigned long caller = (unsigned long) __builtin_return_address(0);
#endif
    int count,chr_count;

	/*
	 * It's possible to come here directly from a panic-assertion and not
	 * have preempt disabled. Some functions called from here want
	 * preempt to be disabled. No point enabling it later though...
	 */
	preempt_disable();

	bust_spinlocks(1);
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	printk(KERN_EMERG "Kernel panic - not syncing: %s\n",buf);
	bust_spinlocks(0);

#ifdef CONFIG_KERNEL_PANIC_DUMP
	PANIC_Current += sprintf(PANIC_Current,"Kernel panic - not syncing: %s\n",buf);
#ifdef REBOOT_WHEN_PANIC_IN_START_KERNEL
	if(start_kernel_ok < 11) {
		printk("panic in statr_kernel() : %d\n", start_kernel_ok);
		arch_reset('r');	
	}
#endif
	if (!multiple_panic) {
		multiple_panic++;	
		printk(KERN_EMERG "PANIC_DUMP Test : \n");
		count = PANIC_Current - PANIC_Base;
		printk("count : %d\n",count);
		chr_count = 0;
		while(count) 
		{
			memset(buf,0x0,1024);
			if(count > 1024) 
			{
				memcpy(buf,PANIC_Base+chr_count,1024);
				printk("%s",buf);
				chr_count += 1024;
				count -= 1024;
			} 
			else 
			{
				memcpy(buf,PANIC_Base+chr_count,count);
				printk("%s",buf);
				chr_count += count;
				count -= count;
			}
		}

		{
			mm_segment_t old_fs;
			struct file *filp;
			int writelen;
			fl_owner_t id = current->files;
			struct timeval val;
			struct tm *ptm,ptmTemp;
			char dt[35];

			preempt_enable();

			old_fs = get_fs();
			set_fs(KERNEL_DS);

			do_gettimeofday(&val);

			ptm = localtime_r(&val.tv_sec,&ptmTemp);

			memset(dt , 0x00 , sizeof(dt));

			// format : YYMMDDhhmm
			sprintf(dt, "/data/KERNEL_PANIC%04d%02d%02d%02d%02d.txt"
					, ptm->tm_year+1900 , ptm->tm_mon+1 , ptm->tm_mday
					, ptm->tm_hour, ptm->tm_min);

			printk("Panic log file is %s \n",dt);

			count = PANIC_Current - PANIC_Base;
			chr_count = 0;
			filp = filp_open(dt,O_CREAT|O_WRONLY,0666);
			if(filp<0)
				printk("Sorry. Can't creat panic file\n");
			else {
				//				vfs_write(filp, PANIC_Base, strlen(PANIC_Base),
				while(count) {
					memset(buf,0x0,1024);
					if(count > 1024) {
						memcpy(buf,PANIC_Base+chr_count,1024);
						writelen = filp->f_op->write(filp,buf,1024,&filp->f_pos);
						if(writelen == 0)
							printk("Write Error!!\n");
						else
							filp->f_op->flush(filp,id);
						chr_count += 1024;
						count -= 1024;
					} else {
						memcpy(buf,PANIC_Base+chr_count,count);
						writelen = filp->f_op->write(filp,buf,count,&filp->f_pos);
						if(writelen == 0)
							printk("Write Error\n");
						else
							filp->f_op->flush(filp,id);
						chr_count += count;
						count -= count;
					}   
				}
			}
			set_fs(old_fs);
			preempt_disable();
		}

		count = PANIC_Current - PANIC_Base;
		printk("\nPanic Dump END,  panic message size is : %d\n",count);
	}
	else {
#if 0
		/* Reset Target */
#else		
		while (1);
#endif
	}
#endif

	/*
	 * If we have crashed and we have a crash kernel loaded let it handle
	 * everything else.
	 * Do we want to call this before we try to display a message?
	 */
	crash_kexec(NULL);

#ifdef CONFIG_SMP
	/*
	 * Note smp_send_stop is the usual smp shutdown function, which
	 * unfortunately means it may not be hardened to work in a panic
	 * situation.
	 */
	smp_send_stop();
#endif

	atomic_notifier_call_chain(&panic_notifier_list, 0, buf);

	if (!panic_blink)
		panic_blink = no_blink;

	if (panic_timeout > 0) {
		/*
	 	 * Delay timeout seconds before rebooting the machine. 
		 * We can't use the "normal" timers since we just panicked..
	 	 */
		printk(KERN_EMERG "Rebooting in %d seconds..",panic_timeout);
		for (i = 0; i < panic_timeout*1000; ) {
			touch_nmi_watchdog();
			i += panic_blink(i);
			mdelay(1);
			i++;
		}
		/*	This will not be a clean reboot, with everything
		 *	shutting down.  But if there is a chance of
		 *	rebooting the system it will be rebooted.
		 */
		emergency_restart();
	}
#ifdef __sparc__
	{
		extern int stop_a_enabled;
		/* Make sure the user can actually press Stop-A (L1-A) */
		stop_a_enabled = 1;
		printk(KERN_EMERG "Press Stop-A (L1-A) to return to the boot prom\n");
	}
#endif
#if defined(CONFIG_S390)
	disabled_wait(caller);
#endif
	local_irq_enable();
	for (i = 0;;) {
		touch_softlockup_watchdog();
		i += panic_blink(i);
		mdelay(1);
		i++;
	}
}

EXPORT_SYMBOL(panic);

/**
 *	print_tainted - return a string to represent the kernel taint state.
 *
 *  'P' - Proprietary module has been loaded.
 *  'F' - Module has been forcibly loaded.
 *  'S' - SMP with CPUs not designed for SMP.
 *  'R' - User forced a module unload.
 *  'M' - System experienced a machine check exception.
 *  'B' - System has hit bad_page.
 *  'U' - Userspace-defined naughtiness.
 *  'A' - ACPI table overridden.
 *  'W' - Taint on warning.
 *
 *	The string is overwritten by the next call to print_taint().
 */

const char *print_tainted(void)
{
	static char buf[20];
	if (tainted) {
		snprintf(buf, sizeof(buf), "Tainted: %c%c%c%c%c%c%c%c%c%c",
			tainted & TAINT_PROPRIETARY_MODULE ? 'P' : 'G',
			tainted & TAINT_FORCED_MODULE ? 'F' : ' ',
			tainted & TAINT_UNSAFE_SMP ? 'S' : ' ',
			tainted & TAINT_FORCED_RMMOD ? 'R' : ' ',
			tainted & TAINT_MACHINE_CHECK ? 'M' : ' ',
			tainted & TAINT_BAD_PAGE ? 'B' : ' ',
			tainted & TAINT_USER ? 'U' : ' ',
			tainted & TAINT_DIE ? 'D' : ' ',
			tainted & TAINT_OVERRIDDEN_ACPI_TABLE ? 'A' : ' ',
			tainted & TAINT_WARN ? 'W' : ' ');
	}
	else
		snprintf(buf, sizeof(buf), "Not tainted");
	return(buf);
}

void add_taint(unsigned flag)
{
	debug_locks = 0; /* can't trust the integrity of the kernel anymore */
	tainted |= flag;
}
EXPORT_SYMBOL(add_taint);

static int __init pause_on_oops_setup(char *str)
{
	pause_on_oops = simple_strtoul(str, NULL, 0);
	return 1;
}
__setup("pause_on_oops=", pause_on_oops_setup);

static void spin_msec(int msecs)
{
	int i;

	for (i = 0; i < msecs; i++) {
		touch_nmi_watchdog();
		mdelay(1);
	}
}

/*
 * It just happens that oops_enter() and oops_exit() are identically
 * implemented...
 */
static void do_oops_enter_exit(void)
{
	unsigned long flags;
	static int spin_counter;

	if (!pause_on_oops)
		return;

	spin_lock_irqsave(&pause_on_oops_lock, flags);
	if (pause_on_oops_flag == 0) {
		/* This CPU may now print the oops message */
		pause_on_oops_flag = 1;
	} else {
		/* We need to stall this CPU */
		if (!spin_counter) {
			/* This CPU gets to do the counting */
			spin_counter = pause_on_oops;
			do {
				spin_unlock(&pause_on_oops_lock);
				spin_msec(MSEC_PER_SEC);
				spin_lock(&pause_on_oops_lock);
			} while (--spin_counter);
			pause_on_oops_flag = 0;
		} else {
			/* This CPU waits for a different one */
			while (spin_counter) {
				spin_unlock(&pause_on_oops_lock);
				spin_msec(1);
				spin_lock(&pause_on_oops_lock);
			}
		}
	}
	spin_unlock_irqrestore(&pause_on_oops_lock, flags);
}

/*
 * Return true if the calling CPU is allowed to print oops-related info.  This
 * is a bit racy..
 */
int oops_may_print(void)
{
	return pause_on_oops_flag == 0;
}

/*
 * Called when the architecture enters its oops handler, before it prints
 * anything.  If this is the first CPU to oops, and it's oopsing the first time
 * then let it proceed.
 *
 * This is all enabled by the pause_on_oops kernel boot option.  We do all this
 * to ensure that oopses don't scroll off the screen.  It has the side-effect
 * of preventing later-oopsing CPUs from mucking up the display, too.
 *
 * It turns out that the CPU which is allowed to print ends up pausing for the
 * right duration, whereas all the other CPUs pause for twice as long: once in
 * oops_enter(), once in oops_exit().
 */
void oops_enter(void)
{
	debug_locks_off(); /* can't trust the integrity of the kernel anymore */
	do_oops_enter_exit();
}

/*
 * 64-bit random ID for oopses:
 */
static u64 oops_id;

static int init_oops_id(void)
{
	if (!oops_id)
		get_random_bytes(&oops_id, sizeof(oops_id));

	return 0;
}
late_initcall(init_oops_id);

static void print_oops_end_marker(void)
{
	init_oops_id();
	printk(KERN_WARNING "---[ end trace %016llx ]---\n",
		(unsigned long long)oops_id);
}

/*
 * Called when the architecture exits its oops handler, after printing
 * everything.
 */
void oops_exit(void)
{
	do_oops_enter_exit();
	print_oops_end_marker();
}

#ifdef WANT_WARN_ON_SLOWPATH
void warn_on_slowpath(const char *file, int line)
{
	char function[KSYM_SYMBOL_LEN];
	unsigned long caller = (unsigned long) __builtin_return_address(0);
	sprint_symbol(function, caller);

	printk(KERN_WARNING "------------[ cut here ]------------\n");
	printk(KERN_WARNING "WARNING: at %s:%d %s()\n", file,
		line, function);
	print_modules();
	dump_stack();
	print_oops_end_marker();
	add_taint(TAINT_WARN);
}
EXPORT_SYMBOL(warn_on_slowpath);


void warn_slowpath(const char *file, int line, const char *fmt, ...)
{
	va_list args;
	char function[KSYM_SYMBOL_LEN];
	unsigned long caller = (unsigned long)__builtin_return_address(0);
	sprint_symbol(function, caller);

	printk(KERN_WARNING "------------[ cut here ]------------\n");
	printk(KERN_WARNING "WARNING: at %s:%d %s()\n", file,
		line, function);
	va_start(args, fmt);
	vprintk(fmt, args);
	va_end(args);

	print_modules();
	dump_stack();
	print_oops_end_marker();
	add_taint(TAINT_WARN);
}
EXPORT_SYMBOL(warn_slowpath);
#endif

#ifdef CONFIG_CC_STACKPROTECTOR
/*
 * Called when gcc's -fstack-protector feature is used, and
 * gcc detects corruption of the on-stack canary value
 */
void __stack_chk_fail(void)
{
	panic("stack-protector: Kernel stack is corrupted");
}
EXPORT_SYMBOL(__stack_chk_fail);
#endif
