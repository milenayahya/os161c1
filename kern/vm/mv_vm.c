/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>

#include <coremap.h>
//#include <vm.h>
#include <segments.h>
#include <addrspace.h>
#include <vm_tlb.h>
#include <pt.h>
#include <swapfile.h>
#include "opt-swapping.h"
#include "opt-on_demand.h"
#include "opt-tlb_management.h"
#include "vmstats.h"

#define OPT_SWAPPING 1
/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 *
 * NOTE: it's been found over the years that students often begin on
 * the VM assignment by copying dumbvm.c and trying to improve it.
 * This is not recommended. dumbvm is (more or less intentionally) not
 * a good design reference. The first recommendation would be: do not
 * look at dumbvm at all. The second recommendation would be: if you
 * do, be sure to review it from the perspective of comparing it to
 * what a VM system is supposed to do, and understanding what corners
 * it's cutting (there are many) and why, and more importantly, how.
 */

/* under dumbvm, always have 72k of user stack */
/* (this must be > 64K so argument blocks of size ARG_MAX will fit) */
#define DUMBVM_STACKPAGES    18

/* G.Cabodi: set DUMBVM_WITH_FREE
 *  - 0: original dumbvm
 *  - 1: support for alloc/free
 */
#define DUMBVM_WITH_FREE 1

#define OPT_PAGING 1 //TO REMOVE
#define OPT_ON_DEMAND 1


#define LOAD_PAGE 1
#define SWAP_IN_PAGE 2
/*
 * Wrap ram_stealmem in a spinlock.
 */
struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;


/* G.Cabodi - support for free/alloc */

struct spinlock freemem_lock = SPINLOCK_INITIALIZER;

unsigned char *freeRamFrames = NULL;
unsigned long *allocSize = NULL;
int nRamFrames = 0;

int allocTableActive = 0;

int isTableActive () {
  int active;
  spinlock_acquire(&freemem_lock);
  active = allocTableActive;
  spinlock_release(&freemem_lock);
  return active;
}

void
vm_bootstrap(void)
{
  int i;
  nRamFrames = ((int)ram_getsize())/PAGE_SIZE;  
  /* alloc freeRamFrame and allocSize */  
  freeRamFrames = kmalloc(sizeof(unsigned char)*nRamFrames);
  if (freeRamFrames==NULL) return;  
  allocSize     = kmalloc(sizeof(unsigned long)*nRamFrames);
  if (allocSize==NULL) {    
    /* reset to disable this vm management */
    freeRamFrames = NULL; return;
  }
  for (i=0; i<nRamFrames; i++) {    
    freeRamFrames[i] = (unsigned char)0;
    allocSize[i]     = 0;  
  }
  spinlock_acquire(&freemem_lock);
  allocTableActive = 1;
  spinlock_release(&freemem_lock);
}

static paddr_t replace_page(paddr_t *ptable){
    paddr_t paddr;
	(void) ptable;
	paddr=dequeue_page();
    
    return paddr;
}

int page_fault(vaddr_t fault_aligned, vaddr_t vbase, vaddr_t vtop, struct segment *seg , paddr_t *dest_paddr){
    paddr_t paddr;

	(void)vtop;
    //you are in the right segment, find the paddr in page table
    long frame=(long) ((fault_aligned - vbase)/PAGE_SIZE);
    paddr = seg->ptable[frame];

    #if OPT_ON_DEMAND
    //paddr not found in PT, try to load page to memory and update PT

    if (paddr == 0){ //the page is not already loaded in memory
        paddr=getuserppage();
        #if OPT_SWAPPING
        //could not load page in memory as memory is full, need to swap
        if(paddr == 0)
        {
            paddr=replace_page(seg->ptable);
            if(paddr==0){
                panic("Problem with swap out");
            }
            
        }
        #endif
        KASSERT((paddr&PAGE_FRAME )== paddr);
        seg->ptable[frame]=paddr;
        queue_page(fault_aligned,seg);
		*dest_paddr=paddr;
		return LOAD_PAGE;
        
    }else if((paddr&PAGE_SWAPPED)!=0){ //the page is swapped out and we need to swap it in
        //In case we want to delete read_only pages, we must look for a free page before
        paddr_t new_paddr;
        new_paddr=getuserppage();
        if(new_paddr==0)
            new_paddr=replace_page(seg->ptable);
        
        swap_in(new_paddr, (off_t) paddr& (~PAGE_SWAPPED));
        seg->ptable[frame]=new_paddr;
		*dest_paddr=new_paddr;
        queue_page(fault_aligned,seg);
		return SWAP_IN_PAGE;
    }
	#endif
	*dest_paddr=paddr;
    return 0;
}

/*int page_fault_stack(vaddr_t fault_aligned, vaddr_t vbase, vaddr_t vtop, paddr_t* stacktable){
    paddr_t paddr;
    //you are in the right segment, find the paddr in page table
    long frame=(long) ((fault_aligned - vbase)/PAGE_SIZE);
    paddr = stacktable[frame];

    #if OPT_ON_DEMAND
    //paddr not found in PT, try to load page to memory and update PT

    if (paddr == 0){ //the page is not already loaded in memory
        paddr=getuserppage();
        #if OPT_SWAPPING
        //could not load page in memory as memory is full, need to swap
        if(paddr == 0)
        {
            paddr=replace_page(seg);
            if(paddr==0){
                panic("Problem with swap out");
            }
            
        }
        #endif
        KASSERT(paddr&PAGE_FRAME == paddr);
        stacktable[frame]=paddr;
        bzero((void *) PADDR_TO_KVADDR(paddr), PAGE_SIZE);
        queue_page(frame);
        
    }else if(paddr&PAGE_SWAPPED!=0){ //the page is swapped out and we need to swap it in
        //In case we want to delete read_only pages, we must look for a free page before
        paddr_t new_paddr;
        new_paddr=getuserppage();
        if(new_paddr==0)
            new_paddr=replace_page(seg);
        
        swap_in(new_paddr, (off_t) paddr& (~PAGE_SWAPPED));
        stacktable[frame]=new_paddr;
        queue_page(frame);
    }
	#endif
    return 0;
}*/


/*
 * Check if we're in a context that can sleep. While most of the
 * operations in dumbvm don't in fact sleep, in a real VM system many
 * of them would. In those, assert that sleeping is ok. This helps
 * avoid the situation where syscall-layer code that works ok with
 * dumbvm starts blowing up during the VM assignment.
 */
static void
dumbvm_can_sleep(void)
{
	if (CURCPU_EXISTS()) {
		/* must not hold spinlocks */
		KASSERT(curcpu->c_spinlocks == 0);

		/* must not be in an interrupt handler */
		KASSERT(curthread->t_in_interrupt == 0);
	}
}

static paddr_t 
getfreeppages(unsigned long npages) {
  paddr_t addr;	
  long i, first, found, np = (long)npages;

  if (!isTableActive()) return 0; 
  spinlock_acquire(&freemem_lock);
  for (i=0,first=found=-1; i<nRamFrames; i++) {
    if (freeRamFrames[i]) {
      if (i==0 || !freeRamFrames[i-1]) 
        first = i; /* set first free in an interval */   
      if (i-first+1 >= np) {
        found = first;
        break;
      }
    }
  }
	
  if (found>=0) {
    for (i=found; i<found+np; i++) {
      freeRamFrames[i] = (unsigned char)0;
    }
    allocSize[found] = np;
    addr = (paddr_t) found*PAGE_SIZE;
  }
  else {
    addr = 0;
  }

  spinlock_release(&freemem_lock);

  return addr;
}

static paddr_t
getppages(unsigned long npages)
{
  paddr_t addr;

  /* try freed pages first */
  addr = getfreeppages(npages);
  if (addr == 0) {
    /* call stealmem */
    spinlock_acquire(&stealmem_lock);
    addr = ram_stealmem(npages);
    spinlock_release(&stealmem_lock);
  }
  if (addr!=0 && isTableActive()) {
    spinlock_acquire(&freemem_lock);
    allocSize[addr/PAGE_SIZE] = npages;
    spinlock_release(&freemem_lock);
  } 

  return addr;
}

static
int 
freeppages(paddr_t addr, unsigned long npages){
  long i, first, np=(long)npages;	

  if (!isTableActive()) return 0; 
  first = addr/PAGE_SIZE;
  KASSERT(allocSize!=NULL);
  KASSERT(nRamFrames>first);

  spinlock_acquire(&freemem_lock);
  for (i=first; i<first+np; i++) {
    freeRamFrames[i] = (unsigned char)1;
  }
  spinlock_release(&freemem_lock);

  return 1;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(unsigned npages)
{
	paddr_t pa;

	dumbvm_can_sleep();
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void 
free_kpages(vaddr_t addr){
  if (isTableActive()) {
    paddr_t paddr = addr - MIPS_KSEG0;
    long first = paddr/PAGE_SIZE;	
    KASSERT(allocSize!=NULL);
    KASSERT(nRamFrames>first);
    freeppages(paddr, allocSize[first]);	
  }
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
 {
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop, fault_aligned;
	paddr_t paddr;
	int i;
	uint32_t ehi, elo;
	struct addrspace *as;
	int spl;

	fault_aligned=faultaddress&PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", fault_aligned);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		panic("dumbvm: got VM_FAULT_READONLY\n");
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	}

	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	as = proc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	#if OPT_PAGING
	KASSERT(as->as_vbase1!=0);
	KASSERT(as->as_ptable1!=0);
	KASSERT(as->as_npages1!=0);
	KASSERT(as->as_vbase2!=0);
	KASSERT(as->as_ptable2!=0);
	KASSERT(as->as_npages2!=0);
	KASSERT(as->as_stackpbase!=0);
	KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	//KASSERT((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

   

	if (fault_aligned >= vbase1 && fault_aligned < vtop1) {
        int result=page_fault(fault_aligned, vbase1, vtop1,&(as->seg1),&paddr);
		if(result==LOAD_PAGE)
			load_page(&(as->seg1),faultaddress, paddr,1);
	}
	else if (fault_aligned >= vbase2 && fault_aligned < vtop2) {
        int result=page_fault(fault_aligned, vbase2, vtop2, &(as->seg2), &paddr);
		if(result==LOAD_PAGE)
			load_page(&(as->seg2),faultaddress, paddr,1);
	
	}
	else if (fault_aligned >= stackbase && fault_aligned < stacktop) {
		int result=page_fault(fault_aligned, stackbase, stacktop, &(as->stackseg),&paddr);
		if(result==LOAD_PAGE){
			bzero((void *)PADDR_TO_KVADDR(paddr), PAGE_SIZE);
		}
	}
	else {
		return EFAULT;
	}
	
	#else
	/* Assert that the address space has been set up properly. */
	KASSERT(as->as_vbase1 != 0);
	KASSERT(as->as_pbase1 != 0);
	KASSERT(as->as_npages1 != 0);
	KASSERT(as->as_vbase2 != 0);
	KASSERT(as->as_pbase2 != 0);
	KASSERT(as->as_npages2 != 0);
	KASSERT(as->as_stackpbase != 0);
	KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	KASSERT((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	KASSERT((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
	KASSERT((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		paddr = (faultaddress - vbase1) + as->as_pbase1;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		paddr = (faultaddress - vbase2) + as->as_pbase2;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		paddr = (faultaddress - stackbase) + as->as_stackpbase;
	}
	else {
		return EFAULT;
	}

	if(paddr==0){
		//load page
	}
	#endif
	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	tlb_faults++;
	for (i=0; i< NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = fault_aligned;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", fault_aligned, paddr);
		tlb_write(ehi, elo, i);
		tlb_faults_with_free++;
		splx(spl);
		return 0;
	}

	//we have reached the end of the TLB and it is full, need a victim
	#if OPT_TLB_MANAGEMENT
	ehi = fault_aligned;
	elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
	int victim = tlb_get_victim();
	tlb_write(ehi, elo, victim);
	tlb_faults_with_replace++;
	kprintf("tlb entry replaced\n");
	splx(spl);
	#endif
	return 0;

	/*kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
	splx(spl);
	return EFAULT;
	*/
}

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	bzero(as,sizeof(struct addrspace));

	if (as==NULL) {
		return NULL;
	}
	#if OPT_PAGING

	as->as_vbase1 = 0;
	as->as_ptable1 = 0;
	as->as_npages1 = 0;

	as->as_vbase2 = 0;
	as->as_ptable2 = 0;
	as->as_npages2 = 0;

	as->as_stackpbase = 0;
	as->initialized=0;
	#if OPT_ON_DEMAND
	// as->seg_header1=NULL;
	as->seg1.vbaseaddr=0;
	as->seg1.npages=0;
	as->seg1.ptable=0;
	as->seg1.filesize=0;
	as->seg1.memsize=0;
	as->seg1.offset=0;
	as->seg1.elf_node=0;
	as->seg1.flags=0;

	as->seg2.vbaseaddr=0;
	as->seg2.npages=0;
	as->seg2.ptable=0;
	as->seg2.filesize=0;
	as->seg2.memsize=0;
	as->seg2.offset=0;
	as->seg2.elf_node=0;
	as->seg2.flags=0;
	// as->seg_header2=NULL;
	#endif
	#else

	as->as_vbase1 = 0;
	as->as_pbase1 = 0;
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	as->as_pbase2 = 0;
	as->as_npages2 = 0;
	as->as_stackpbase = 0;

	

	#endif

	return as;
}

void as_destroy(struct addrspace *as){
  dumbvm_can_sleep();
  #if OPT_PAGING
  	destroy_ptable(as->as_ptable1, as->as_npages1);
	destroy_ptable(as->as_ptable2, as->as_npages2);
	destroy_ptable(as->as_stackpbase, DUMBVM_STACKPAGES);
  #else
  freeppages(as->as_pbase1, as->as_npages1);
  freeppages(as->as_pbase2, as->as_npages2);
  freeppages(as->as_stackpbase, DUMBVM_STACKPAGES);
  #endif
  clear_queue(as);
  
  kfree(as);
}

void
as_activate(void)
{
	int i, spl;
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		return;
	}

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}
	tlb_invalidations++;
	splx(spl);
}

void
as_deactivate(void)
{
	/* nothing */
}

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages;

	dumbvm_can_sleep();

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	/* We don't use these - all pages are read-write */
	(void)readable;
	(void)writeable;
	(void)executable;

	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;

		as->seg1.vbaseaddr=vaddr;
		as->seg1.npages=npages;

		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;

		as->seg2.vbaseaddr=vaddr;
		as->seg2.npages=npages;
		return 0;
	}


	/*
	 * Support for more than two regions is not available.
	 */
	kprintf("dumbvm: Warning: too many regions\n");
	return ENOSYS;
}

int as_define_segment(struct addrspace *as, vaddr_t vbase, off_t offset, size_t memsize, size_t filesize, struct vnode* elf_node, uint32_t flags){
	if(as->initialized==0){
		as->seg1.vbaseaddr=vbase;
		as->seg1.offset=offset;
		as->seg1.memsize=memsize;
		as->seg1.filesize=filesize;
		as->seg1.elf_node=elf_node;
		as->seg1.flags=flags;
		as->initialized++;
		return 0;
	}else if(as->initialized==1){
		as->seg2.vbaseaddr=vbase;
		as->seg2.offset=offset;
		as->seg2.memsize=memsize;
		as->seg2.filesize=filesize;
		as->seg2.elf_node=elf_node;
		as->seg2.flags=flags;
		as->initialized++;
		return 0;
	}
	else
		return 1;
}

#if OPT_ON_DEMAND
#else
#if OPT_PAGING
static void as_zero_region(paddr_t * ptable, unsigned long npages){
	for(unsigned long i=0;i<npages;i++){
		bzero((void *) PADDR_TO_KVADDR(ptable[i]),PAGE_SIZE);
	}
}
#else
static
void
as_zero_region(paddr_t paddr, unsigned long npages)
{
	bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);


}
#endif
#endif
int
as_prepare_load(struct addrspace *as)
{

#if OPT_PAGING
	KASSERT(as->as_ptable1 == 0);
	KASSERT(as->as_ptable2 == 0);
	KASSERT(as->as_stackpbase == 0);
	#if OPT_ON_DEMAND
		as->as_ptable1 = (paddr_t *)kmalloc(sizeof(paddr_t)*as->as_npages1);
		bzero((void *)PADDR_TO_KVADDR(as->as_ptable1), sizeof(paddr_t)*as->as_npages1);
		as->seg1.ptable=as->as_ptable1;
		if (as->as_ptable1 == 0) {
			return ENOMEM;
		}

		as->as_ptable2 =(paddr_t *)kmalloc(sizeof(paddr_t)*as->as_npages2);
		bzero((void *)PADDR_TO_KVADDR(as->as_ptable2), sizeof(paddr_t)*as->as_npages2 );
		as->seg2.ptable=as->as_ptable2;
		if (as->as_ptable2 == 0) {
			return ENOMEM;
		}

		as->as_stackpbase = (paddr_t *)kmalloc(sizeof(paddr_t)*DUMBVM_STACKPAGES);
		bzero((void *)PADDR_TO_KVADDR(as->as_stackpbase), sizeof(paddr_t)*DUMBVM_STACKPAGES);
		as->stackseg.ptable= as->as_stackpbase;
		if (as->as_stackpbase == 0) {
			return ENOMEM;
		}
		// as_zero_region(as->as_ptable1, as->as_npages1);
		// as_zero_region(as->as_ptable2, as->as_npages2);
		// as_zero_region(as->as_stackpbase, DUMBVM_STACKPAGES);
	#else
		as->as_ptable1 = getuserppages(as->as_npages1);
		if (as->as_ptable1 == 0) {
			return ENOMEM;
		}

		as->as_ptable2 = getuserppages(as->as_npages2);
		if (as->as_ptable2 == 0) {
			return ENOMEM;
		}

		as->as_stackpbase = getuserppages(DUMBVM_STACKPAGES);
		if (as->as_stackpbase == 0) {
			return ENOMEM;
		}
		as_zero_region(as->as_ptable1, as->as_npages1);
		as_zero_region(as->as_ptable2, as->as_npages2);
		as_zero_region(as->as_stackpbase, DUMBVM_STACKPAGES);
	#endif


#else
	KASSERT(as->as_pbase1 == 0);
	KASSERT(as->as_pbase2 == 0);
	KASSERT(as->as_stackpbase == 0);

	dumbvm_can_sleep();

	as->as_pbase1 = getppages(as->as_npages1);
	if (as->as_pbase1 == 0) {
		return ENOMEM;
	}

	as->as_pbase2 = getppages(as->as_npages2);
	if (as->as_pbase2 == 0) {
		return ENOMEM;
	}

	as->as_stackpbase = getppages(DUMBVM_STACKPAGES);
	if (as->as_stackpbase == 0) {
		return ENOMEM;
	}

	as_zero_region(as->as_pbase1, as->as_npages1);
	as_zero_region(as->as_pbase2, as->as_npages2);
	as_zero_region(as->as_stackpbase, DUMBVM_STACKPAGES);

#endif

	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	dumbvm_can_sleep();
	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	KASSERT(as->as_stackpbase != 0);

	as->stackseg.vbaseaddr=USERSTACK-DUMBVM_STACKPAGES*PAGE_SIZE;
	*stackptr = USERSTACK;
	return 0;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;

	dumbvm_can_sleep();

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

#if OPT_PAGING
	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->seg1=old->seg1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;
	new->seg2=old->seg2;

	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}

	KASSERT(new->as_ptable1 != 0);
	KASSERT(new->as_ptable2 != 0);
	KASSERT(new->as_stackpbase != 0);
	for(unsigned long i=0;i<old->as_npages1;i++){
		memmove((void *)PADDR_TO_KVADDR(new->as_ptable1[i]),
			(const void *)PADDR_TO_KVADDR(old->as_ptable1[i]),
			PAGE_SIZE);
	}
	for(unsigned long i=0;i<old->as_npages2;i++){
		memmove((void *)PADDR_TO_KVADDR(new->as_ptable2[i]),
			(const void *)PADDR_TO_KVADDR(old->as_ptable2[i]),
			PAGE_SIZE);
	}
	for(unsigned long i=0;i<DUMBVM_STACKPAGES;i++){
		memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase[i]),
			(const void *)PADDR_TO_KVADDR(old->as_stackpbase[i]),
			PAGE_SIZE);
	}
#else

	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;

	/* (Mis)use as_prepare_load to allocate some physical memory. */
	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}

	KASSERT(new->as_pbase1 != 0);
	KASSERT(new->as_pbase2 != 0);
	KASSERT(new->as_stackpbase != 0);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
		(const void *)PADDR_TO_KVADDR(old->as_pbase1),
		old->as_npages1*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
		(const void *)PADDR_TO_KVADDR(old->as_pbase2),
		old->as_npages2*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
		DUMBVM_STACKPAGES*PAGE_SIZE);
#endif
	*ret = new;
	return 0;
}

