#include <swapfile.h>

int swap_init(void){

    int fd;
    fd = vfs_open(SWAP_PATH, O_RDWR, 0, &swapfile);
    swapmap = bitmap_create(SWAP_SIZE/PAGE_SIZE); 
    // we open the swapfile and save it in a global handle vnode: swapfile
    kprintf("Swapping initialized");
    
}

unsigned long swap_out(paddr_t page, off_t *ret_offset){
    int index=-1;
    int result;

    KASSERT(page != 0);
    KASSERT(page & PAGE_FRAME ==  page)

    result=bitmap_alloc(swapmap, &index);
    if(result){
        return result;
    }

    struct uio uio;
    struct iovec iov;

    uio_kinit(&iov, &uio, PADDR_TO_KVADDR((void *) paddr), PAGE_SIZE, index*PAGE_SIZE, UIO_WRITE);
    
    result=VOP_WRITE(swapfile, uio);
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
    KASSERT(page & PAGE_FRAME ==  page)
    KASSERT(offset_swapfile & PAGE_FRAME == offset_swapfile);
    KASSERT(offset_swapfile < SWAP_SIZE);

    unsigned int index = offset_swapfile/ PAGE_SIZE; //index in swapmap of the page to be swapped in, should be used to verify entry in bitmap exists
    
    //in reality need to use spinlocks to read the bitmap

    KASSERT(bitmap_isset(swapmap, index)==1);
    
    struct uio uio;
    struct iovec iov;
    
    uio_kinit(&iov, &uio, (void *) PADDR_TO_KVADDR (page), PAGE_SIZE, offset_swapfile, UIO_READ);
    VOP_READ(swapfile, &uio);

}

int swap_clean(){
    bzero(PADDR_TO_KVADDR((void *)swapmap.v), sizeof(WORD_TYPE)*swapmap.nbits);
    
}

