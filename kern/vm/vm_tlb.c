#include <types.h>
#include <mips/tlb.h>
#include <vm.h>




// Round robin algorithm for victim selection in the tlb
int tlb_get_victim(void){
   // int tlb_entries[NUM_TLB];
    static unsigned next_victim = 0;
    unsigned int victim =  next_victim;
    next_victim = (next_victim + 1) % NUM_TLB;
    return victim;

}