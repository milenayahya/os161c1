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