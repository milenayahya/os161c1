
#include <vm_tlb.h>



// Round robin algorithm for victim selection in the tlb
int tlb_get_victim(void){
   // int tlb_entries[NUM_TLB];
    static unsigned next_victim = 0;
    unsigned int victim =  next_victim;
    next_victim = (next_victim + 1) % NUM_TLB;
    return victim;

}

int invalidate_entry(vaddr_t entryhi, paddr_t entrylow){
    int t=tlb_probe(entryhi, entrylow);
    if(t<0)
        return t;
        
    tlb_write(TLBHI_INVALID(t), TLBLO_INVALID(), t);
    return 0;
}

int insert_tlb_entry(vaddr_t fault_aligned, paddr_t paddr, bool readonly)
{
    KASSERT((paddr&PAGE_FRAME)==paddr);
    int i;
	uint32_t ehi, elo;
    int spl;
    spl = splhigh();
    for (i=0; i< NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = fault_aligned;
		elo = paddr  | TLBLO_VALID;
		if(!readonly)
			elo|=TLBLO_DIRTY;
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

}