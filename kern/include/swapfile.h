#ifndef _SWAPFILE_H_
#define _SWAPFILE_H_

#include <vfs.h>
#include <bitmap.h>
#include <types.h>
#include <spinlock.h>
#include <uio.h>
#include <vnode.h>
#include <kern/errno.h>
#include <vm.h>

#define SWAP_SIZE 9*1024*1024
#define SWAP_PATH "lhdr0raw:"

#define PAGE_SWAPPED 1

struct bitmap *swapmap;
struct vnode *swapfile;
struct spinlock swapfile_lock;

int swap_init();
int swap_in(paddr_t page, off_t offset_swapfile);
unsigned long swap_out(paddr_t page,off_t *ret_offset);
void swap_clear(off_t index);
int swap_clean();

int swap_shutdown();

#endif