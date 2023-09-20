#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <mips/vm.h>
#include <vm.h>
#include <addrspace.h>

paddr_t *init_ptable(unsigned long npages);
paddr_t getuserppage(void);
paddr_t getfreeuserpage(void);
paddr_t alloc_upage_from_ram(void);
//void destroy_ptable(paddr_t *ptable);