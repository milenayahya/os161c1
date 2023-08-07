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

int load_page(struct addrspace *as, struct vnode *v,
	     off_t offset, vaddr_t vaddr, vaddr_t vbase ,
	     int is_executable);