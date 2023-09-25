#ifndef _SEGMENTS_H_
#define _SEGMENTS_H_

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

#define MV_VM_STACKPAGES 18

int
load_page(struct segment *seg,
	     vaddr_t vaddr, paddr_t paddr,
	     int is_executable);

void seg_init(struct segment *seg);

#endif