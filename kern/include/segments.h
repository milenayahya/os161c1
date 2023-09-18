#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vnode.h>
#include <elf.h>
#include "opt-on_demand.h"

int
load_page(struct segment *seg,
	     vaddr_t vaddr, paddr_t paddr
	     int is_executable);