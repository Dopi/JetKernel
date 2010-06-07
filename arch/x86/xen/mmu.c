/*
 * Xen mmu operations
 *
 * This file contains the various mmu fetch and update operations.
 * The most important job they must perform is the mapping between the
 * domain's pfn and the overall machine mfns.
 *
 * Xen allows guests to directly update the pagetable, in a controlled
 * fashion.  In other words, the guest modifies the same pagetable
 * that the CPU actually uses, which eliminates the overhead of having
 * a separate shadow pagetable.
 *
 * In order to allow this, it falls on the guest domain to map its
 * notion of a "physical" pfn - which is just a domain-local linear
 * address - into a real "machine address" which the CPU's MMU can
 * use.
 *
 * A pgd_t/pmd_t/pte_t will typically contain an mfn, and so can be
 * inserted directly into the pagetable.  When creating a new
 * pte/pmd/pgd, it converts the passed pfn into an mfn.  Conversely,
 * when reading the content back with __(pgd|pmd|pte)_val, it converts
 * the mfn back into a pfn.
 *
 * The other constraint is that all pages which make up a pagetable
 * must be mapped read-only in the guest.  This prevents uncontrolled
 * guest updates to the pagetable.  Xen strictly enforces this, and
 * will disallow any pagetable update which will end up mapping a
 * pagetable page RW, and will disallow using any writable page as a
 * pagetable.
 *
 * Naively, when loading %cr3 with the base of a new pagetable, Xen
 * would need to validate the whole pagetable before going on.
 * Naturally, this is quite slow.  The solution is to "pin" a
 * pagetable, which enforces all the constraints on the pagetable even
 * when it is not actively in use.  This menas that Xen can be assured
 * that it is still valid when you do load it into %cr3, and doesn't
 * need to revalidate it.
 *
 * Jeremy Fitzhardinge <jeremy@xensource.com>, XenSource Inc, 2007
 */
#include <linux/sched.h>
#include <linux/highmem.h>
#include <linux/debugfs.h>
#include <linux/bug.h>

#include <asm/pgtable.h>
#include <asm/tlbflush.h>
#include <asm/fixmap.h>
#include <asm/mmu_context.h>
#include <asm/paravirt.h>
#include <asm/linkage.h>

#include <asm/xen/hypercall.h>
#include <asm/xen/hypervisor.h>

#include <xen/page.h>
#include <xen/interface/xen.h>

#include "multicalls.h"
#include "mmu.h"
#include "debugfs.h"

#define MMU_UPDATE_HISTO	30

#ifdef CONFIG_XEN_DEBUG_FS

static struct {
	u32 pgd_update;
	u32 pgd_update_pinned;
	u32 pgd_update_batched;

	u32 pud_update;
	u32 pud_update_pinned;
	u32 pud_update_batched;

	u32 pmd_update;
	u32 pmd_update_pinned;
	u32 pmd_update_batched;

	u32 pte_update;
	u32 pte_update_pinned;
	u32 pte_update_batched;

	u32 mmu_update;
	u32 mmu_update_extended;
	u32 mmu_update_histo[MMU_UPDATE_HISTO];

	u32 prot_commit;
	u32 prot_commit_batched;

	u32 set_pte_at;
	u32 set_pte_at_batched;
	u32 set_pte_at_pinned;
	u32 set_pte_at_current;
	u32 set_pte_at_kernel;
} mmu_stats;

static u8 zero_stats;

static inline void check_zero(void)
{
	if (unlikely(zero_stats)) {
		memset(&mmu_stats, 0, sizeof(mmu_stats));
		zero_stats = 0;
	}
}

#define ADD_STATS(elem, val)			\
	do { check_zero(); mmu_stats.elem += (val); } while(0)

#else  /* !CONFIG_XEN_DEBUG_FS */

#define ADD_STATS(elem, val)	do { (void)(val); } while(0)

#endif /* CONFIG_XEN_DEBUG_FS */

/*
 * Just beyond the highest usermode address.  STACK_TOP_MAX has a
 * redzone above it, so round it up to a PGD boundary.
 */
#define USER_LIMIT	((STACK_TOP_MAX + PGDIR_SIZE - 1) & PGDIR_MASK)


#define P2M_ENTRIES_PER_PAGE	(PAGE_SIZE / sizeof(unsigned long))
#define TOP_ENTRIES		(MAX_DOMAIN_PAGES / P2M_ENTRIES_PER_PAGE)

/* Placeholder for holes in the address space */
static unsigned long p2m_missing[P2M_ENTRIES_PER_PAGE] __page_aligned_data =
		{ [ 0 ... P2M_ENTRIES_PER_PAGE-1 ] = ~0UL };

 /* Array of pointers to pages containing p2m entries */
static unsigned long *p2m_top[TOP_ENTRIES] __page_aligned_data =
		{ [ 0 ... TOP_ENTRIES - 1] = &p2m_missing[0] };

/* Arrays of p2m arrays expressed in mfns used for save/restore */
static unsigned long p2m_top_mfn[TOP_ENTRIES] __page_aligned_bss;

static unsigned long p2m_top_mfn_list[TOP_ENTRIES / P2M_ENTRIES_PER_PAGE]
	__page_aligned_bss;

static inline unsigned p2m_top_index(unsigned long pfn)
{
	BUG_ON(pfn >= MAX_DOMAIN_PAGES);
	return pfn / P2M_ENTRIES_PER_PAGE;
}

static inline unsigned p2m_index(unsigned long pfn)
{
	return pfn % P2M_ENTRIES_PER_PAGE;
}

/* Build the parallel p2m_top_mfn structures */
void xen_setup_mfn_list_list(void)
{
	unsigned pfn, idx;

	for (pfn = 0; pfn < MAX_DOMAIN_PAGES; pfn += P2M_ENTRIES_PER_PAGE) {
		unsigned topidx = p2m_top_index(pfn);

		p2m_top_mfn[topidx] = virt_to_mfn(p2m_top[topidx]);
	}

	for (idx = 0; idx < ARRAY_SIZE(p2m_top_mfn_list); idx++) {
		unsigned topidx = idx * P2M_ENTRIES_PER_PAGE;
		p2m_top_mfn_list[idx] = virt_to_mfn(&p2m_top_mfn[topidx]);
	}

	BUG_ON(HYPERVISOR_shared_info == &xen_dummy_shared_info);

	HYPERVISOR_shared_info->arch.pfn_to_mfn_frame_list_list =
		virt_to_mfn(p2m_top_mfn_list);
	HYPERVISOR_shared_info->arch.max_pfn = xen_start_info->nr_pages;
}

/* Set up p2m_top to point to the domain-builder provided p2m pages */
void __init xen_build_dynamic_phys_to_machine(void)
{
	unsigned long *mfn_list = (unsigned long *)xen_start_info->mfn_list;
	unsigned long max_pfn = min(MAX_DOMAIN_PAGES, xen_start_info->nr_pages);
	unsigned pfn;

	for (pfn = 0; pfn < max_pfn; pfn += P2M_ENTRIES_PER_PAGE) {
		unsigned topidx = p2m_top_index(pfn);

		p2m_top[topidx] = &mfn_list[pfn];
	}
}

unsigned long get_phys_to_machine(unsigned long pfn)
{
	unsigned topidx, idx;

	if (unlikely(pfn >= MAX_DOMAIN_PAGES))
		return INVALID_P2M_ENTRY;

	topidx = p2m_top_index(pfn);
	idx = p2m_index(pfn);
	return p2m_top[topidx][idx];
}
EXPORT_SYMBOL_GPL(get_phys_to_machine);

static void alloc_p2m(unsigned long **pp, unsigned long *mfnp)
{
	unsigned long *p;
	unsigned i;

	p = (void *)__get_free_page(GFP_KERNEL | __GFP_NOFAIL);
	BUG_ON(p == NULL);

	for (i = 0; i < P2M_ENTRIES_PER_PAGE; i++)
		p[i] = INVALID_P2M_ENTRY;

	if (cmpxchg(pp, p2m_missing, p) != p2m_missing)
		free_page((unsigned long)p);
	else
		*mfnp = virt_to_mfn(p);
}

void set_phys_to_machine(unsigned long pfn, unsigned long mfn)
{
	unsigned topidx, idx;

	if (unlikely(xen_feature(XENFEAT_auto_translated_physmap))) {
		BUG_ON(pfn != mfn && mfn != INVALID_P2M_ENTRY);
		return;
	}

	if (unlikely(pfn >= MAX_DOMAIN_PAGES)) {
		BUG_ON(mfn != INVALID_P2M_ENTRY);
		return;
	}

	topidx = p2m_top_index(pfn);
	if (p2m_top[topidx] == p2m_missing) {
		/* no need to allocate a page to store an invalid entry */
		if (mfn == INVALID_P2M_ENTRY)
			return;
		alloc_p2m(&p2m_top[topidx], &p2m_top_mfn[topidx]);
	}

	idx = p2m_index(pfn);
	p2m_top[topidx][idx] = mfn;
}

xmaddr_t arbitrary_virt_to_machine(void *vaddr)
{
	unsigned long address = (unsigned long)vaddr;
	unsigned int level;
	pte_t *pte;
	unsigned offset;

	/*
	 * if the PFN is in the linear mapped vaddr range, we can just use
	 * the (quick) virt_to_machine() p2m lookup
	 */
	if (virt_addr_valid(vaddr))
		return virt_to_machine(vaddr);

	/* otherwise we have to do a (slower) full page-table walk */

	pte = lookup_address(address, &level);
	BUG_ON(pte == NULL);
	offset = address & ~PAGE_MASK;
	return XMADDR(((phys_addr_t)pte_mfn(*pte) << PAGE_SHIFT) + offset);
}

void make_lowmem_page_readonly(void *vaddr)
{
	pte_t *pte, ptev;
	unsigned long address = (unsigned long)vaddr;
	unsigned int level;

	pte = lookup_address(address, &level);
	BUG_ON(pte == NULL);

	ptev = pte_wrprotect(*pte);

	if (HYPERVISOR_update_va_mapping(address, ptev, 0))
		BUG();
}

void make_lowmem_page_readwrite(void *vaddr)
{
	pte_t *pte, ptev;
	unsigned long address = (unsigned long)vaddr;
	unsigned int level;

	pte = lookup_address(address, &level);
	BUG_ON(pte == NULL);

	ptev = pte_mkwrite(*pte);

	if (HYPERVISOR_update_va_mapping(address, ptev, 0))
		BUG();
}


static bool xen_page_pinned(void *ptr)
{
	struct page *page = virt_to_page(ptr);

	return PagePinned(page);
}

static void xen_extend_mmu_update(const struct mmu_update *update)
{
	struct multicall_space mcs;
	struct mmu_update *u;

	mcs = xen_mc_extend_args(__HYPERVISOR_mmu_update, sizeof(*u));

	if (mcs.mc != NULL) {
		ADD_STATS(mmu_update_extended, 1);
		ADD_STATS(mmu_update_histo[mcs.mc->args[1]], -1);

		mcs.mc->args[1]++;

		if (mcs.mc->args[1] < MMU_UPDATE_HISTO)
			ADD_STATS(mmu_update_histo[mcs.mc->args[1]], 1);
		else
			ADD_STATS(mmu_update_histo[0], 1);
	} else {
		ADD_STATS(mmu_update, 1);
		mcs = __xen_mc_entry(sizeof(*u));
		MULTI_mmu_update(mcs.mc, mcs.args, 1, NULL, DOMID_SELF);
		ADD_STATS(mmu_update_histo[1], 1);
	}

	u = mcs.args;
	*u = *update;
}

void xen_set_pmd_hyper(pmd_t *ptr, pmd_t val)
{
	struct mmu_update u;

	preempt_disable();

	xen_mc_batch();

	/* ptr may be ioremapped for 64-bit pagetable setup */
	u.ptr = arbitrary_virt_to_machine(ptr).maddr;
	u.val = pmd_val_ma(val);
	xen_extend_mmu_update(&u);

	ADD_STATS(pmd_update_batched, paravirt_get_lazy_mode() == PARAVIRT_LAZY_MMU);

	xen_mc_issue(PARAVIRT_LAZY_MMU);

	preempt_enable();
}

void xen_set_pmd(pmd_t *ptr, pmd_t val)
{
	ADD_STATS(pmd_update, 1);

	/* If page is not pinned, we can just update the entry
	   directly */
	if (!xen_page_pinned(ptr)) {
		*ptr = val;
		return;
	}

	ADD_STATS(pmd_update_pinned, 1);

	xen_set_pmd_hyper(ptr, val);
}

/*
 * Associate a virtual page frame with a given physical page frame
 * and protection flags for that frame.
 */
void set_pte_mfn(unsigned long vaddr, unsigned long mfn, pgprot_t flags)
{
	set_pte_vaddr(vaddr, mfn_pte(mfn, flags));
}

void xen_set_pte_at(struct mm_struct *mm, unsigned long addr,
		    pte_t *ptep, pte_t pteval)
{
	/* updates to init_mm may be done without lock */
	if (mm == &init_mm)
		preempt_disable();

	ADD_STATS(set_pte_at, 1);
//	ADD_STATS(set_pte_at_pinned, xen_page_pinned(ptep));
	ADD_STATS(set_pte_at_current, mm == current->mm);
	ADD_STATS(set_pte_at_kernel, mm == &init_mm);

	if (mm == current->mm || mm == &init_mm) {
		if (paravirt_get_lazy_mode() == PARAVIRT_LAZY_MMU) {
			struct multicall_space mcs;
			mcs = xen_mc_entry(0);

			MULTI_update_va_mapping(mcs.mc, addr, pteval, 0);
			ADD_STATS(set_pte_at_batched, 1);
			xen_mc_issue(PARAVIRT_LAZY_MMU);
			goto out;
		} else
			if (HYPERVISOR_update_va_mapping(addr, pteval, 0) == 0)
				goto out;
	}
	xen_set_pte(ptep, pteval);

out:
	if (mm == &init_mm)
		preempt_enable();
}

pte_t xen_ptep_modify_prot_start(struct mm_struct *mm,
				 unsigned long addr, pte_t *ptep)
{
	/* Just return the pte as-is.  We preserve the bits on commit */
	return *ptep;
}

void xen_ptep_modify_prot_commit(struct mm_struct *mm, unsigned long addr,
				 pte_t *ptep, pte_t pte)
{
	struct mmu_update u;

	xen_mc_batch();

	u.ptr = arbitrary_virt_to_machine(ptep).maddr | MMU_PT_UPDATE_PRESERVE_AD;
	u.val = pte_val_ma(pte);
	xen_extend_mmu_update(&u);

	ADD_STATS(prot_commit, 1);
	ADD_STATS(prot_commit_batched, paravirt_get_lazy_mode() == PARAVIRT_LAZY_MMU);

	xen_mc_issue(PARAVIRT_LAZY_MMU);
}

/* Assume pteval_t is equivalent to all the other *val_t types. */
static pteval_t pte_mfn_to_pfn(pteval_t val)
{
	if (val & _PAGE_PRESENT) {
		unsigned long mfn = (val & PTE_PFN_MASK) >> PAGE_SHIFT;
		pteval_t flags = val & PTE_FLAGS_MASK;
		val = ((pteval_t)mfn_to_pfn(mfn) << PAGE_SHIFT) | flags;
	}

	return val;
}

static pteval_t pte_pfn_to_mfn(pteval_t val)
{
	if (val & _PAGE_PRESENT) {
		unsigned long pfn = (val & PTE_PFN_MASK) >> PAGE_SHIFT;
		pteval_t flags = val & PTE_FLAGS_MASK;
		val = ((pteval_t)pfn_to_mfn(pfn) << PAGE_SHIFT) | flags;
	}

	return val;
}

pteval_t xen_pte_val(pte_t pte)
{
	return pte_mfn_to_pfn(pte.pte);
}

pgdval_t xen_pgd_val(pgd_t pgd)
{
	return pte_mfn_to_pfn(pgd.pgd);
}

pte_t xen_make_pte(pteval_t pte)
{
	pte = pte_pfn_to_mfn(pte);
	return native_make_pte(pte);
}

pgd_t xen_make_pgd(pgdval_t pgd)
{
	pgd = pte_pfn_to_mfn(pgd);
	return native_make_pgd(pgd);
}

pmdval_t xen_pmd_val(pmd_t pmd)
{
	return pte_mfn_to_pfn(pmd.pmd);
}

void xen_set_pud_hyper(pud_t *ptr, pud_t val)
{
	struct mmu_update u;

	preempt_disable();

	xen_mc_batch();

	/* ptr may be ioremapped for 64-bit pagetable setup */
	u.ptr = arbitrary_virt_to_machine(ptr).maddr;
	u.val = pud_val_ma(val);
	xen_extend_mmu_update(&u);

	ADD_STATS(pud_update_batched, paravirt_get_lazy_mode() == PARAVIRT_LAZY_MMU);

	xen_mc_issue(PARAVIRT_LAZY_MMU);

	preempt_enable();
}

void xen_set_pud(pud_t *ptr, pud_t val)
{
	ADD_STATS(pud_update, 1);

	/* If page is not pinned, we can just update the entry
	   directly */
	if (!xen_page_pinned(ptr)) {
		*ptr = val;
		return;
	}

	ADD_STATS(pud_update_pinned, 1);

	xen_set_pud_hyper(ptr, val);
}

void xen_set_pte(pte_t *ptep, pte_t pte)
{
	ADD_STATS(pte_update, 1);
//	ADD_STATS(pte_update_pinned, xen_page_pinned(ptep));
	ADD_STATS(pte_update_batched, paravirt_get_lazy_mode() == PARAVIRT_LAZY_MMU);

#ifdef CONFIG_X86_PAE
	ptep->pte_high = pte.pte_high;
	smp_wmb();
	ptep->pte_low = pte.pte_low;
#else
	*ptep = pte;
#endif
}

#ifdef CONFIG_X86_PAE
void xen_set_pte_atomic(pte_t *ptep, pte_t pte)
{
	set_64bit((u64 *)ptep, native_pte_val(pte));
}

void xen_pte_clear(struct mm_struct *mm, unsigned long addr, pte_t *ptep)
{
	ptep->pte_low = 0;
	smp_wmb();		/* make sure low gets written first */
	ptep->pte_high = 0;
}

void xen_pmd_clear(pmd_t *pmdp)
{
	set_pmd(pmdp, __pmd(0));
}
#endif	/* CONFIG_X86_PAE */

pmd_t xen_make_pmd(pmdval_t pmd)
{
	pmd = pte_pfn_to_mfn(pmd);
	return native_make_pmd(pmd);
}

#if PAGETABLE_LEVELS == 4
pudval_t xen_pud_val(pud_t pud)
{
	return pte_mfn_to_pfn(pud.pud);
}

pud_t xen_make_pud(pudval_t pud)
{
	pud = pte_pfn_to_mfn(pud);

	return native_make_pud(pud);
}

pgd_t *xen_get_user_pgd(pgd_t *pgd)
{
	pgd_t *pgd_page = (pgd_t *)(((unsigned long)pgd) & PAGE_MASK);
	unsigned offset = pgd - pgd_page;
	pgd_t *user_ptr = NULL;

	if (offset < pgd_index(USER_LIMIT)) {
		struct page *page = virt_to_page(pgd_page);
		user_ptr = (pgd_t *)page->private;
		if (user_ptr)
			user_ptr += offset;
	}

	return user_ptr;
}

static void __xen_set_pgd_hyper(pgd_t *ptr, pgd_t val)
{
	struct mmu_update u;

	u.ptr = virt_to_machine(ptr).maddr;
	u.val = pgd_val_ma(val);
	xen_extend_mmu_update(&u);
}

/*
 * Raw hypercall-based set_pgd, intended for in early boot before
 * there's a page structure.  This implies:
 *  1. The only existing pagetable is the kernel's
 *  2. It is always pinned
 *  3. It has no user pagetable attached to it
 */
void __init xen_set_pgd_hyper(pgd_t *ptr, pgd_t val)
{
	preempt_disable();

	xen_mc_batch();

	__xen_set_pgd_hyper(ptr, val);

	xen_mc_issue(PARAVIRT_LAZY_MMU);

	preempt_enable();
}

void xen_set_pgd(pgd_t *ptr, pgd_t val)
{
	pgd_t *user_ptr = xen_get_user_pgd(ptr);

	ADD_STATS(pgd_update, 1);

	/* If page is not pinned, we can just update the entry
	   directly */
	if (!xen_page_pinned(ptr)) {
		*ptr = val;
		if (user_ptr) {
			WARN_ON(xen_page_pinned(user_ptr));
			*user_ptr = val;
		}
		return;
	}

	ADD_STATS(pgd_update_pinned, 1);
	ADD_STATS(pgd_update_batched, paravirt_get_lazy_mode() == PARAVIRT_LAZY_MMU);

	/* If it's pinned, then we can at least batch the kernel and
	   user updates together. */
	xen_mc_batch();

	__xen_set_pgd_hyper(ptr, val);
	if (user_ptr)
		__xen_set_pgd_hyper(user_ptr, val);

	xen_mc_issue(PARAVIRT_LAZY_MMU);
}
#endif	/* PAGETABLE_LEVELS == 4 */

/*
 * (Yet another) pagetable walker.  This one is intended for pinning a
 * pagetable.  This means that it walks a pagetable and calls the
 * callback function on each page it finds making up the page table,
 * at every level.  It walks the entire pagetable, but it only bothers
 * pinning pte pages which are below limit.  In the normal case this
 * will be STACK_TOP_MAX, but at boot we need to pin up to
 * FIXADDR_TOP.
 *
 * For 32-bit the important bit is that we don't pin beyond there,
 * because then we start getting into Xen's ptes.
 *
 * For 64-bit, we must skip the Xen hole in the middle of the address
 * space, just after the big x86-64 virtual hole.
 */
static int __xen_pgd_walk(struct mm_struct *mm, pgd_t *pgd,
			  int (*func)(struct mm_struct *mm, struct page *,
				      enum pt_level),
			  unsigned long limit)
{
	int flush = 0;
	unsigned hole_low, hole_high;
	unsigned pgdidx_limit, pudidx_limit, pmdidx_limit;
	unsigned pgdidx, pudidx, pmdidx;

	/* The limit is the last byte to be touched */
	limit--;
	BUG_ON(limit >= FIXADDR_TOP);

	if (xen_feature(XENFEAT_auto_translated_physmap))
		return 0;

	/*
	 * 64-bit has a great big hole in the middle of the address
	 * space, which contains the Xen mappings.  On 32-bit these
	 * will end up making a zero-sized hole and so is a no-op.
	 */
	hole_low = pgd_index(USER_LIMIT);
	hole_high = pgd_index(PAGE_OFFSET);

	pgdidx_limit = pgd_index(limit);
#if PTRS_PER_PUD > 1
	pudidx_limit = pud_index(limit);
#else
	pudidx_limit = 0;
#endif
#if PTRS_PER_PMD > 1
	pmdidx_limit = pmd_index(limit);
#else
	pmdidx_limit = 0;
#endif

	for (pgdidx = 0; pgdidx <= pgdidx_limit; pgdidx++) {
		pud_t *pud;

		if (pgdidx >= hole_low && pgdidx < hole_high)
			continue;

		if (!pgd_val(pgd[pgdidx]))
			continue;

		pud = pud_offset(&pgd[pgdidx], 0);

		if (PTRS_PER_PUD > 1) /* not folded */
			flush |= (*func)(mm, virt_to_page(pud), PT_PUD);

		for (pudidx = 0; pudidx < PTRS_PER_PUD; pudidx++) {
			pmd_t *pmd;

			if (pgdidx == pgdidx_limit &&
			    pudidx > pudidx_limit)
				goto out;

			if (pud_none(pud[pudidx]))
				continue;

			pmd = pmd_offset(&pud[pudidx], 0);

			if (PTRS_PER_PMD > 1) /* not folded */
				flush |= (*func)(mm, virt_to_page(pmd), PT_PMD);

			for (pmdidx = 0; pmdidx < PTRS_PER_PMD; pmdidx++) {
				struct page *pte;

				if (pgdidx == pgdidx_limit &&
				    pudidx == pudidx_limit &&
				    pmdidx > pmdidx_limit)
					goto out;

				if (pmd_none(pmd[pmdidx]))
					continue;

				pte = pmd_page(pmd[pmdidx]);
				flush |= (*func)(mm, pte, PT_PTE);
			}
		}
	}

out:
	/* Do the top level last, so that the callbacks can use it as
	   a cue to do final things like tlb flushes. */
	flush |= (*func)(mm, virt_to_page(pgd), PT_PGD);

	return flush;
}

static int xen_pgd_walk(struct mm_struct *mm,
			int (*func)(struct mm_struct *mm, struct page *,
				    enum pt_level),
			unsigned long limit)
{
	return __xen_pgd_walk(mm, mm->pgd, func, limit);
}

/* If we're using split pte locks, then take the page's lock and
   return a pointer to it.  Otherwise return NULL. */
static spinlock_t *xen_pte_lock(struct page *page, struct mm_struct *mm)
{
	spinlock_t *ptl = NULL;

#if USE_SPLIT_PTLOCKS
	ptl = __pte_lockptr(page);
	spin_lock_nest_lock(ptl, &mm->page_table_lock);
#endif

	return ptl;
}

static void xen_pte_unlock(void *v)
{
	spinlock_t *ptl = v;
	spin_unlock(ptl);
}

static void xen_do_pin(unsigned level, unsigned long pfn)
{
	struct mmuext_op *op;
	struct multicall_space mcs;

	mcs = __xen_mc_entry(sizeof(*op));
	op = mcs.args;
	op->cmd = level;
	op->arg1.mfn = pfn_to_mfn(pfn);
	MULTI_mmuext_op(mcs.mc, op, 1, NULL, DOMID_SELF);
}

static int xen_pin_page(struct mm_struct *mm, struct page *page,
			enum pt_level level)
{
	unsigned pgfl = TestSetPagePinned(page);
	int flush;

	if (pgfl)
		flush = 0;		/* already pinned */
	else if (PageHighMem(page))
		/* kmaps need flushing if we found an unpinned
		   highpage */
		flush = 1;
	else {
		void *pt = lowmem_page_address(page);
		unsigned long pfn = page_to_pfn(page);
		struct multicall_space mcs = __xen_mc_entry(0);
		spinlock_t *ptl;

		flush = 0;

		/*
		 * We need to hold the pagetable lock between the time
		 * we make the pagetable RO and when we actually pin
		 * it.  If we don't, then other users may come in and
		 * attempt to update the pagetable by writing it,
		 * which will fail because the memory is RO but not
		 * pinned, so Xen won't do the trap'n'emulate.
		 *
		 * If we're using split pte locks, we can't hold the
		 * entire pagetable's worth of locks during the
		 * traverse, because we may wrap the preempt count (8
		 * bits).  The solution is to mark RO and pin each PTE
		 * page while holding the lock.  This means the number
		 * of locks we end up holding is never more than a
		 * batch size (~32 entries, at present).
		 *
		 * If we're not using split pte locks, we needn't pin
		 * the PTE pages independently, because we're
		 * protected by the overall pagetable lock.
		 */
		ptl = NULL;
		if (level == PT_PTE)
			ptl = xen_pte_lock(page, mm);

		MULTI_update_va_mapping(mcs.mc, (unsigned long)pt,
					pfn_pte(pfn, PAGE_KERNEL_RO),
					level == PT_PGD ? UVMF_TLB_FLUSH : 0);

		if (ptl) {
			xen_do_pin(MMUEXT_PIN_L1_TABLE, pfn);

			/* Queue a deferred unlock for when this batch
			   is completed. */
			xen_mc_callback(xen_pte_unlock, ptl);
		}
	}

	return flush;
}

/* This is called just after a mm has been created, but it has not
   been used yet.  We need to make sure that its pagetable is all
   read-only, and can be pinned. */
static void __xen_pgd_pin(struct mm_struct *mm, pgd_t *pgd)
{
	vm_unmap_aliases();

	xen_mc_batch();

	if (__xen_pgd_walk(mm, pgd, xen_pin_page, USER_LIMIT)) {
		/* re-enable interrupts for flushing */
		xen_mc_issue(0);

		kmap_flush_unused();

		xen_mc_batch();
	}

#ifdef CONFIG_X86_64
	{
		pgd_t *user_pgd = xen_get_user_pgd(pgd);

		xen_do_pin(MMUEXT_PIN_L4_TABLE, PFN_DOWN(__pa(pgd)));

		if (user_pgd) {
			xen_pin_page(mm, virt_to_page(user_pgd), PT_PGD);
			xen_do_pin(MMUEXT_PIN_L4_TABLE,
				   PFN_DOWN(__pa(user_pgd)));
		}
	}
#else /* CONFIG_X86_32 */
#ifdef CONFIG_X86_PAE
	/* Need to make sure unshared kernel PMD is pinnable */
	xen_pin_page(mm, pgd_page(pgd[pgd_index(TASK_SIZE)]),
		     PT_PMD);
#endif
	xen_do_pin(MMUEXT_PIN_L3_TABLE, PFN_DOWN(__pa(pgd)));
#endif /* CONFIG_X86_64 */
	xen_mc_issue(0);
}

static void xen_pgd_pin(struct mm_struct *mm)
{
	__xen_pgd_pin(mm, mm->pgd);
}

/*
 * On save, we need to pin all pagetables to make sure they get their
 * mfns turned into pfns.  Search the list for any unpinned pgds and pin
 * them (unpinned pgds are not currently in use, probably because the
 * process is under construction or destruction).
 *
 * Expected to be called in stop_machine() ("equivalent to taking
 * every spinlock in the system"), so the locking doesn't really
 * matter all that much.
 */
void xen_mm_pin_all(void)
{
	unsigned long flags;
	struct page *page;

	spin_lock_irqsave(&pgd_lock, flags);

	list_for_each_entry(page, &pgd_list, lru) {
		if (!PagePinned(page)) {
			__xen_pgd_pin(&init_mm, (pgd_t *)page_address(page));
			SetPageSavePinned(page);
		}
	}

	spin_unlock_irqrestore(&pgd_lock, flags);
}

/*
 * The init_mm pagetable is really pinned as soon as its created, but
 * that's before we have page structures to store the bits.  So do all
 * the book-keeping now.
 */
static __init int xen_mark_pinned(struct mm_struct *mm, struct page *page,
				  enum pt_level level)
{
	SetPagePinned(page);
	return 0;
}

void __init xen_mark_init_mm_pinned(void)
{
	xen_pgd_walk(&init_mm, xen_mark_pinned, FIXADDR_TOP);
}

static int xen_unpin_page(struct mm_struct *mm, struct page *page,
			  enum pt_level level)
{
	unsigned pgfl = TestClearPagePinned(page);

	if (pgfl && !PageHighMem(page)) {
		void *pt = lowmem_page_address(page);
		unsigned long pfn = page_to_pfn(page);
		spinlock_t *ptl = NULL;
		struct multicall_space mcs;

		/*
		 * Do the converse to pin_page.  If we're using split
		 * pte locks, we must be holding the lock for while
		 * the pte page is unpinned but still RO to prevent
		 * concurrent updates from seeing it in this
		 * partially-pinned state.
		 */
		if (level == PT_PTE) {
			ptl = xen_pte_lock(page, mm);

			if (ptl)
				xen_do_pin(MMUEXT_UNPIN_TABLE, pfn);
		}

		mcs = __xen_mc_entry(0);

		MULTI_update_va_mapping(mcs.mc, (unsigned long)pt,
					pfn_pte(pfn, PAGE_KERNEL),
					level == PT_PGD ? UVMF_TLB_FLUSH : 0);

		if (ptl) {
			/* unlock when batch completed */
			xen_mc_callback(xen_pte_unlock, ptl);
		}
	}

	return 0;		/* never need to flush on unpin */
}

/* Release a pagetables pages back as normal RW */
static void __xen_pgd_unpin(struct mm_struct *mm, pgd_t *pgd)
{
	xen_mc_batch();

	xen_do_pin(MMUEXT_UNPIN_TABLE, PFN_DOWN(__pa(pgd)));

#ifdef CONFIG_X86_64
	{
		pgd_t *user_pgd = xen_get_user_pgd(pgd);

		if (user_pgd) {
			xen_do_pin(MMUEXT_UNPIN_TABLE,
				   PFN_DOWN(__pa(user_pgd)));
			xen_unpin_page(mm, virt_to_page(user_pgd), PT_PGD);
		}
	}
#endif

#ifdef CONFIG_X86_PAE
	/* Need to make sure unshared kernel PMD is unpinned */
	xen_unpin_page(mm, pgd_page(pgd[pgd_index(TASK_SIZE)]),
		       PT_PMD);
#endif

	__xen_pgd_walk(mm, pgd, xen_unpin_page, USER_LIMIT);

	xen_mc_issue(0);
}

static void xen_pgd_unpin(struct mm_struct *mm)
{
	__xen_pgd_unpin(mm, mm->pgd);
}

/*
 * On resume, undo any pinning done at save, so that the rest of the
 * kernel doesn't see any unexpected pinned pagetables.
 */
void xen_mm_unpin_all(void)
{
	unsigned long flags;
	struct page *page;

	spin_lock_irqsave(&pgd_lock, flags);

	list_for_each_entry(page, &pgd_list, lru) {
		if (PageSavePinned(page)) {
			BUG_ON(!PagePinned(page));
			__xen_pgd_unpin(&init_mm, (pgd_t *)page_address(page));
			ClearPageSavePinned(page);
		}
	}

	spin_unlock_irqrestore(&pgd_lock, flags);
}

void xen_activate_mm(struct mm_struct *prev, struct mm_struct *next)
{
	spin_lock(&next->page_table_lock);
	xen_pgd_pin(next);
	spin_unlock(&next->page_table_lock);
}

void xen_dup_mmap(struct mm_struct *oldmm, struct mm_struct *mm)
{
	spin_lock(&mm->page_table_lock);
	xen_pgd_pin(mm);
	spin_unlock(&mm->page_table_lock);
}


#ifdef CONFIG_SMP
/* Another cpu may still have their %cr3 pointing at the pagetable, so
   we need to repoint it somewhere else before we can unpin it. */
static void drop_other_mm_ref(void *info)
{
	struct mm_struct *mm = info;
	struct mm_struct *active_mm;

#ifdef CONFIG_X86_64
	active_mm = read_pda(active_mm);
#else
	active_mm = __get_cpu_var(cpu_tlbstate).active_mm;
#endif

	if (active_mm == mm)
		leave_mm(smp_processor_id());

	/* If this cpu still has a stale cr3 reference, then make sure
	   it has been flushed. */
	if (x86_read_percpu(xen_current_cr3) == __pa(mm->pgd)) {
		load_cr3(swapper_pg_dir);
		arch_flush_lazy_cpu_mode();
	}
}

static void xen_drop_mm_ref(struct mm_struct *mm)
{
	cpumask_var_t mask;
	unsigned cpu;

	if (current->active_mm == mm) {
		if (current->mm == mm)
			load_cr3(swapper_pg_dir);
		else
			leave_mm(smp_processor_id());
		arch_flush_lazy_cpu_mode();
	}

	/* Get the "official" set of cpus referring to our pagetable. */
	if (!alloc_cpumask_var(&mask, GFP_ATOMIC)) {
		for_each_online_cpu(cpu) {
			if (!cpumask_test_cpu(cpu, &mm->cpu_vm_mask)
			    && per_cpu(xen_current_cr3, cpu) != __pa(mm->pgd))
				continue;
			smp_call_function_single(cpu, drop_other_mm_ref, mm, 1);
		}
		return;
	}
	cpumask_copy(mask, &mm->cpu_vm_mask);

	/* It's possible that a vcpu may have a stale reference to our
	   cr3, because its in lazy mode, and it hasn't yet flushed
	   its set of pending hypercalls yet.  In this case, we can
	   look at its actual current cr3 value, and force it to flush
	   if needed. */
	for_each_online_cpu(cpu) {
		if (per_cpu(xen_current_cr3, cpu) == __pa(mm->pgd))
			cpumask_set_cpu(cpu, mask);
	}

	if (!cpumask_empty(mask))
		smp_call_function_many(mask, drop_other_mm_ref, mm, 1);
	free_cpumask_var(mask);
}
#else
static void xen_drop_mm_ref(struct mm_struct *mm)
{
	if (current->active_mm == mm)
		load_cr3(swapper_pg_dir);
}
#endif

/*
 * While a process runs, Xen pins its pagetables, which means that the
 * hypervisor forces it to be read-only, and it controls all updates
 * to it.  This means that all pagetable updates have to go via the
 * hypervisor, which is moderately expensive.
 *
 * Since we're pulling the pagetable down, we switch to use init_mm,
 * unpin old process pagetable and mark it all read-write, which
 * allows further operations on it to be simple memory accesses.
 *
 * The only subtle point is that another CPU may be still using the
 * pagetable because of lazy tlb flushing.  This means we need need to
 * switch all CPUs off this pagetable before we can unpin it.
 */
void xen_exit_mmap(struct mm_struct *mm)
{
	get_cpu();		/* make sure we don't move around */
	xen_drop_mm_ref(mm);
	put_cpu();

	spin_lock(&mm->page_table_lock);

	/* pgd may not be pinned in the error exit path of execve */
	if (xen_page_pinned(mm->pgd))
		xen_pgd_unpin(mm);

	spin_unlock(&mm->page_table_lock);
}

#ifdef CONFIG_XEN_DEBUG_FS

static struct dentry *d_mmu_debug;

static int __init xen_mmu_debugfs(void)
{
	struct dentry *d_xen = xen_init_debugfs();

	if (d_xen == NULL)
		return -ENOMEM;

	d_mmu_debug = debugfs_create_dir("mmu", d_xen);

	debugfs_create_u8("zero_stats", 0644, d_mmu_debug, &zero_stats);

	debugfs_create_u32("pgd_update", 0444, d_mmu_debug, &mmu_stats.pgd_update);
	debugfs_create_u32("pgd_update_pinned", 0444, d_mmu_debug,
			   &mmu_stats.pgd_update_pinned);
	debugfs_create_u32("pgd_update_batched", 0444, d_mmu_debug,
			   &mmu_stats.pgd_update_pinned);

	debugfs_create_u32("pud_update", 0444, d_mmu_debug, &mmu_stats.pud_update);
	debugfs_create_u32("pud_update_pinned", 0444, d_mmu_debug,
			   &mmu_stats.pud_update_pinned);
	debugfs_create_u32("pud_update_batched", 0444, d_mmu_debug,
			   &mmu_stats.pud_update_pinned);

	debugfs_create_u32("pmd_update", 0444, d_mmu_debug, &mmu_stats.pmd_update);
	debugfs_create_u32("pmd_update_pinned", 0444, d_mmu_debug,
			   &mmu_stats.pmd_update_pinned);
	debugfs_create_u32("pmd_update_batched", 0444, d_mmu_debug,
			   &mmu_stats.pmd_update_pinned);

	debugfs_create_u32("pte_update", 0444, d_mmu_debug, &mmu_stats.pte_update);
//	debugfs_create_u32("pte_update_pinned", 0444, d_mmu_debug,
//			   &mmu_stats.pte_update_pinned);
	debugfs_create_u32("pte_update_batched", 0444, d_mmu_debug,
			   &mmu_stats.pte_update_pinned);

	debugfs_create_u32("mmu_update", 0444, d_mmu_debug, &mmu_stats.mmu_update);
	debugfs_create_u32("mmu_update_extended", 0444, d_mmu_debug,
			   &mmu_stats.mmu_update_extended);
	xen_debugfs_create_u32_array("mmu_update_histo", 0444, d_mmu_debug,
				     mmu_stats.mmu_update_histo, 20);

	debugfs_create_u32("set_pte_at", 0444, d_mmu_debug, &mmu_stats.set_pte_at);
	debugfs_create_u32("set_pte_at_batched", 0444, d_mmu_debug,
			   &mmu_stats.set_pte_at_batched);
	debugfs_create_u32("set_pte_at_current", 0444, d_mmu_debug,
			   &mmu_stats.set_pte_at_current);
	debugfs_create_u32("set_pte_at_kernel", 0444, d_mmu_debug,
			   &mmu_stats.set_pte_at_kernel);

	debugfs_create_u32("prot_commit", 0444, d_mmu_debug, &mmu_stats.prot_commit);
	debugfs_create_u32("prot_commit_batched", 0444, d_mmu_debug,
			   &mmu_stats.prot_commit_batched);

	return 0;
}
fs_initcall(xen_mmu_debugfs);

#endif	/* CONFIG_XEN_DEBUG_FS */
