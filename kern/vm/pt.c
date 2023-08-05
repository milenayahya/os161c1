

#include <pt.h>


/// @brief Inits an empty physical page table
/// @param npages 
/// @return the empty array
paddr_t *init_ptable(unsigned long npages){
    
    paddr_t *frames;
    frames=(paddr_t *)kmalloc(sizeof(paddr_t)*npages);
    
    bzero(frames, sizeof(paddr_t)*npages);
    return frames;
    
}

/// @brief Allocs a single user page
/// @return the address of the allocated frame
paddr_t getuserppage(paddr_t * ptable){
    paddr_t frame = getfreeuserpage();
    if (frame == 0)
        //We have to steal memory

}

paddr_t getfreeuserpage(){ //looks for an available frame and allocates it to a page

    if (!isTableActive()) return 0;
    spinlock_acquire(&freemem_lock); //mutual exclusion 


    for(i =0; i<nRamFrames; i++){
        if(freeRamFrames[i]==1){
            
            freeRamFrames[i]=0;
            spinlock_release(&freemem_lock);
            return (paddr_t) i*PAGE_SIZE;
    
        }
    }
    spinlock_release(&freemem_lock);
    return 0;
}