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

paddr_t * getfreeuserpages(unsigned long *npages);

paddr_t *getuserppages(unsigned long npages);

paddr_t *alloc_upages_from_ram(unsigned long npages);

