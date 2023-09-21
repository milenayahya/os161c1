#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <swapfile.h>
#include <mips/tlb.h>
#include <mips/vm.h>
#include <vm_tlb.h>
#include <vm.h>

#include <addrspace.h>

struct page_queue{
    long index;
    struct segment *seg;
    struct page_queue *next;
};


paddr_t *init_ptable(unsigned long npages);
paddr_t getuserppage(void);
paddr_t getfreeuserpage(void);
paddr_t alloc_upage_from_ram(void);
int page_fault(vaddr_t fault_aligned, vaddr_t vbase, vaddr_t vtop, struct segment *ptable, paddr_t *dest_paddr);
void queue_page(vaddr_t frame, struct segment *seg);
paddr_t dequeue_page(void);

void clear_queue(struct addrspace*as);
//void destroy_ptable(paddr_t *ptable);