#include <swapfile.h>


int swap_init(void){

    int result;

    char swapfile_name[32];
    strcpy(swapfile_name, SWAP_PATH);
    // we open the swapfile and save it in a global handle vnode: swapfile
    result = vfs_open(swapfile_name, O_RDWR, 0, &swapfile);
    if(result){
        panic("Couldn't open swapfile");
    }
    //create bitmap for swapfile
    swapmap = bitmap_create(SWAP_SIZE/PAGE_SIZE); 
    
    kprintf("Swapping initialized\n");
    spinlock_init(&swapfile_lock);
    return 0;
}

unsigned long swap_out(paddr_t page, off_t *ret_offset){
    unsigned index=0;
    int result;

    
    spinlock_acquire(&swapfile_lock); //to protect the bitmap "swapmap"
    KASSERT(spinlock_do_i_hold(&swapfile_lock));
    KASSERT(page != 0);
    page&=PAGE_FRAME;
    KASSERT((page & PAGE_FRAME) ==  page);

    result=bitmap_alloc(swapmap, &index);
    spinlock_release(&swapfile_lock);
    if(result){
        return result;  //Will return a ENOSPC in case of cleared bit not found
    }

    
    struct uio uio;
    struct iovec iov;

    uio_kinit(&iov, &uio, (void *) PADDR_TO_KVADDR( page), PAGE_SIZE, index*PAGE_SIZE, UIO_WRITE);
    
    result=VOP_WRITE(swapfile, &uio);
    if(result){
        panic("Result not zero in VOP_WRITE");
        return result;
    }
    if(uio.uio_resid>0){
        kprintf("Short write in swapping: %lu bytes left over\n",
            (unsigned long) uio.uio_resid);
        panic("Couldn't write to swapfile");
        return ENOSPC;
    }

    
    *ret_offset=index*PAGE_SIZE;
    return 0;

}

int swap_in(paddr_t page, off_t offset_swapfile){
    
    KASSERT(page != 0);
    KASSERT((page & PAGE_FRAME )==  page);
    KASSERT((offset_swapfile & PAGE_FRAME )== offset_swapfile);
    KASSERT(offset_swapfile < SWAP_SIZE);

    unsigned int index = offset_swapfile/ PAGE_SIZE; //index in swapmap of the page to be swapped in, should be used to verify entry in bitmap exists
    
    //in reality need to use spinlocks to read the bitmap

    spinlock_acquire(&swapfile_lock);
    if(bitmap_isset(swapmap, index)==0)
    {
        panic("trying to access empty page in swapfile");
    }
    
    //unmark the bitmap
   
    bitmap_unmark(swapmap,index);
    spinlock_release(&swapfile_lock);
    
    struct uio uio;
    struct iovec iov;
    
    uio_kinit(&iov, &uio, (void *) PADDR_TO_KVADDR (page), PAGE_SIZE, offset_swapfile, UIO_READ);
    VOP_READ(swapfile, &uio);
    
    if(uio.uio_resid!=0)
        panic("Couldn't read the whole page from swapfile");

    

    return 0;

}

int swap_clean(void){  //resetting the entries of the bitmap and marking them all free
    spinlock_acquire(&swapfile_lock);
    //bzero((void *)PADDR_TO_KVADDR(swapmap->v), sizeof(WORD_TYPE)* (swapmap->nbits));
    spinlock_release(&swapfile_lock);
    return 0;
}

void swap_clear(off_t swap_page){  //clear particular page
    spinlock_acquire(&swapfile_lock);
    swap_page&=PAGE_FRAME;
    int index=swap_page/PAGE_SIZE;
    KASSERT(bitmap_isset(swapmap, index));
    bitmap_unmark(swapmap, index);

    spinlock_release(&swapfile_lock);
}

int swap_shutdown(void){
    spinlock_acquire(&swapfile_lock);
    bitmap_destroy(swapmap);
    vfs_close(swapfile);
    
    spinlock_release(&swapfile_lock);

    spinlock_cleanup(&swapfile_lock);
    return 0;
}


