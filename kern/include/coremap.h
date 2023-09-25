#ifndef _COREMAP_H_
#define _COREMAP_H_


#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <vm.h>
#include <addrspace.h>
#include <swapfile.h>
#include <mips/vm.h>
#include <vm_tlb.h>

struct page_queue{
    long index;
    struct segment *seg;
    struct page_queue *next;
};

paddr_t * getfreeuserpages(unsigned long *npages, paddr_t *firstpaddr);

paddr_t *getfreeppagesCONTIG(unsigned long npages);

paddr_t *getuserppages(unsigned long npages);

int alloc_upages_from_ram(unsigned long npages, unsigned long offset, paddr_t *ptable);

void destroy_ptable(paddr_t *table, long npages);

void queue_page(vaddr_t frame, struct segment *seg);

paddr_t dequeue_page(void);

void clear_queue(struct addrspace*as);

#endif