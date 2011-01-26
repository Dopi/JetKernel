#ifndef _KERNEL_SEC_COMMON_H_
#define _KERNEL_SEC_COMMON_H_

#ifdef CONFIG_KERNEL_DEBUG_SEC

#include <asm/io.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <mach/map.h>
#include <plat/regs-clock.h>


#define UPLOAD_DRAM_BASE 0x50000000
#define UPLOAD_DRAM_SIZE 0xD000000
#define UPLOAD_INFO_SIZE 0x1000
#define UPLOAD_INFO_VIRT_ADDR p_upload_info
#define UPLOAD_INFO_PHYS_ADDR (UPLOAD_DRAM_BASE+UPLOAD_DRAM_SIZE-0x1000-UPLOAD_INFO_SIZE) //-0x1000 is for sec_log_buf


// MAGIC_CODE in LOKE
#define UPLOAD_MAGIC_VIRT_ADDR  UPLOAD_INFO_VIRT_ADDR
#define UPLOAD_MAGIC_NUMBER     0x66262564

// INFORMATION REGISTER
#define UPLOAD_CAUSE_REG_ADDR (UPLOAD_INFO_VIRT_ADDR+4) // Magic code for upload cause.
#define UPLOAD_AP_CP_NOTIFY_CODE 0x54534552

// GETLOG
#define UPLOAD_GETLOG_INFO_ADDR (UPLOAD_INFO_VIRT_ADDR+0xC0)
#define UPLOAD_GETLOG_INFO_SIZE 0x40

//WDOG register
#define S3C_PA_WDT 0x7E004000 //arch dependent

typedef struct tag_mmu_info
{	
	int SCTLR;
	int TTBR0;
	int TTBR1;
	int TTBCR;
	int DACR;
	int DFSR;
	int DFAR;
	int IFSR;
	int IFAR;
	int DAFSR;
	int IAFSR;
	int PMRRR;
	int NMRRR;
	int FCSEPID;
	int CONTEXT;
	int URWTPID;
	int UROTPID;
	int POTPIDR;
}t_kernel_sec_mmu_info;

/*ARM CORE regs mapping structure*/
typedef struct
{
	/* COMMON */
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;

	/* SVC */
	unsigned int r13_svc;
	unsigned int r14_svc;
	unsigned int spsr_svc;

	/* PC & CPSR */
	unsigned int pc;
	unsigned int cpsr;
	
	/* USR/SYS */
	unsigned int r13_usr;
	unsigned int r14_usr;

	/* FIQ */
	unsigned int r8_fiq;
	unsigned int r9_fiq;
	unsigned int r10_fiq;
	unsigned int r11_fiq;
	unsigned int r12_fiq;
	unsigned int r13_fiq;
	unsigned int r14_fiq;
	unsigned int spsr_fiq;

	/* IRQ */
	unsigned int r13_irq;
	unsigned int r14_irq;
	unsigned int spsr_irq;

	/* MON */
	unsigned int r13_mon;
	unsigned int r14_mon;
	unsigned int spsr_mon;

	/* ABT */
	unsigned int r13_abt;
	unsigned int r14_abt;
	unsigned int spsr_abt;

	/* UNDEF */
	unsigned int r13_und;
	unsigned int r14_und;
	unsigned int spsr_und;

}t_kernel_sec_arm_core_regsiters;

typedef enum
{
	UPLOAD_CAUSE_INIT           = 0x00000000,
	UPLOAD_CAUSE_KERNEL_PANIC   = 0x11111111,
	UPLOAD_CAUSE_FORCED_UPLOAD  = 0x22222222,
	UPLOAD_CAUSE_CP_ERROR_FATAL = 0x33333333,
	UPLOAD_CAUSE_CP_LOCKUP		= 0x33334444,
	UPLOAD_CAUSE_CP_WDOG		= 0x33335555,

	BLK_UART_MSG_FOR_FACTRST_2ND_ACK = 0x88888888,
}kernel_sec_upload_cause_type;


extern void __iomem * kernel_sec_viraddr_wdt_reset_reg;
extern void kernel_sec_map_wdog_reg(void);

extern void kernel_sec_set_upload_magic_number(void);
extern void kernel_sec_set_upload_cause(kernel_sec_upload_cause_type uploadType);
extern void kernel_sec_clear_upload_magic_number(void);

extern void kernel_sec_hw_reset(bool bSilentReset);
extern void kernel_sec_init(void);

extern void kernel_sec_get_core_reg_dump(t_kernel_sec_arm_core_regsiters* regs);
extern int  kernel_sec_get_mmu_reg_dump(t_kernel_sec_mmu_info *mmu_info);
extern void kernel_sec_save_final_context(void);

#endif // CONFIG_KERNEL_DEBUG_SEC
#endif /* _KERNEL_SEC_COMMON_H_ */
