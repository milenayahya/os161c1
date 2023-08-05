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
#include <mips/vm.h>

paddr_t * getfreeuserpages(unsigned long *npages, paddr_t *firstpaddr);

paddr_t *getfreeppagesCONTIG(unsigned long npages);

paddr_t *getuserppages(unsigned long npages);

int alloc_upages_from_ram(unsigned long npages, unsigned long offset, paddr_t *ptable);

void destroy_ptable(paddr_t *table, long npages);

