
#include <vmstats.h>

int stats_init(void){
    tlb_faults=0;
    tlb_faults_with_free=0;
    tlb_faults_with_replace=0;
    tlb_invalidations=0;
    tlb_reloads=0;
    page_faults_disk=0;
    page_faults_elf=0;
    page_faults_swap=0;
    page_faults_zeroed=0;
    swap_writes=0;

    return 1;
}

void print_stats(void){
    kprintf("TLB Faults: %d\n", tlb_faults);
    kprintf("TLB Faults with Free: %d\n",tlb_faults_with_free);
    kprintf("TLB Faults with Replace: %d\n", tlb_faults_with_replace);
    kprintf("TLB invalidations: %d\n", tlb_invalidations);
    kprintf("TLB Reloads: %d\n",tlb_reloads);
    kprintf("Disk Page Faults: %d\n", page_faults_disk);
    kprintf("Page Faults from ELF: %d\n",page_faults_elf);
    kprintf("Page Faults from SWAPFILE: %d\n", page_faults_swap);
    kprintf("Page Faults Zeroed: %d\n",page_faults_zeroed);
    kprintf("SWAPFILE writes: %d\n",swap_writes);
    kprintf("Now we verify\n");
    kprintf("TLB faults = TLB faults with Free + TLB faults with Replace = %d + %d = %d\n", tlb_faults_with_free,tlb_faults_with_replace,tlb_faults_with_free+tlb_faults_with_replace);
    kprintf("Disk Page Faults = Page Faults from ELF + Page Faults from SWAPFILE= %d + %d = %d\n",page_faults_elf,page_faults_swap,page_faults_elf+page_faults_swap);
    kprintf("TLB faults are the sum of TLB Reloads (no page fault) and Page Faults:\n");
    kprintf("TLB Faults = TLB Reloads + Page Faults Zeroed + Disk Page Faults =  %d + %d + %d = %d\n", tlb_reloads, page_faults_zeroed, page_faults_disk, tlb_reloads+page_faults_zeroed+page_faults_disk);
}