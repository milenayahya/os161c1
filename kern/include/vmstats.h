#include <types.h>
#include <lib.h>


int tlb_faults;
int tlb_faults_with_free;
int tlb_faults_with_replace;
int tlb_invalidations;
int tlb_reloads;
int page_faults_disk;
int page_faults_elf;
int page_faults_swap;
int page_faults_zeroed;
int swap_writes;

int stats_init(void);
void print_stats(void);
