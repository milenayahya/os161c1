#ifndef _SWAPFILE_H_
#define _SWAPFILE_H_

#include <types.h>
#include <lib.h>
#include <kern/fcntl.h>
#include <vfs.h>
#include <bitmap.h>

#include <spinlock.h>
#include <uio.h>
#include <vnode.h>
#include <kern/errno.h>
#include <vm.h>

#define SWAP_SIZE 9*1024*1024
#define SWAP_PATH "lhd0raw:"

#define PAGE_SWAPPED 1

#ifndef WORD_TYPE
#define WORD_TYPE unsigned char
#endif

#ifndef BITS_PER_WORD
#define BITS_PER_WORD   (CHAR_BIT)
#endif

struct bitmap *swapmap;
struct vnode *swapfile;
struct spinlock swapfile_lock;

int swap_init(void);
int swap_in(paddr_t page, off_t offset_swapfile);
unsigned long swap_out(paddr_t page,off_t *ret_offset);
void swap_clear(off_t index);
int swap_clean(void);

int swap_shutdown(void);

#endif