#ifndef _ASM_X86_MMU_CONTEXT_64_H
#define _ASM_X86_MMU_CONTEXT_64_H

#include <asm/pda.h>

static inline void enter_lazy_tlb(struct mm_struct *mm, struct task_struct *tsk)
{
#ifdef CONFIG_SMP
	if (read_pda(mmu_state) == TLBSTATE_OK)
		write_pda(mmu_state, TLBSTATE_LAZY);
#endif
}

static inline void switch_mm(struct mm_struct *prev, struct mm_struct *next,
			     struct task_struct *tsk)
{
	unsigned cpu = smp_processor_id();
	if (likely(prev != next)) {
		/* stop flush ipis for the previous mm */
		cpu_clear(cpu, prev->cpu_vm_mask);
#ifdef CONFIG_SMP
		write_pda(mmu_state, TLBSTATE_OK);
		write_pda(active_mm, next);
#endif
		cpu_set(cpu, next->cpu_vm_mask);
		load_cr3(next->pgd);

		if (unlikely(next->context.ldt != prev->context.ldt))
			load_LDT_nolock(&next->context);
	}
#ifdef CONFIG_SMP
	else {
		write_pda(mmu_state, TLBSTATE_OK);
		if (read_pda(active_mm) != next)
			BUG();
		if (!cpu_test_and_set(cpu, next->cpu_vm_mask)) {
			/* We were in lazy tlb mode and leave_mm disabled
			 * tlb flush IPI delivery. We must reload CR3
			 * to make sure to use no freed page tables.
			 */
			load_cr3(next->pgd);
			load_LDT_nolock(&next->context);
		}
	}
#endif
}

#define deactivate_mm(tsk, mm)			\
do {						\
	load_gs_index(0);			\
	asm volatile("movl %0,%%fs"::"r"(0));	\
} while (0)

#endif /* _ASM_X86_MMU_CONTEXT_64_H */
