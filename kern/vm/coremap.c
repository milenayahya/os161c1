#include <coremap.h>

//gets noncontiguous free physical pages
paddr_t * getfreeuserpages(unsigned long *npages) {
    
    long i, j, np = (long)*npages;
    paddr_t *firstpaddr;
    long found =0;  //it is the number of free pages found

    firstpaddr = (paddr_t *)kmalloc(sizeof(long)* (*npages));
    
    if (!isTableActive()) return 0;

    spinlock_acquire(&freemem_lock); //mutual exclusion 
   
    j=0;

    for(i =0; i<nRamFrames; i++){
        if(freeRamFrames[i]==1){
            found++; //add the nb of pages found
            //add to array every new available page
            //transfrom values in firstpaddr to paddr_t
            
            firstpaddr[j]=(paddr_t) i*PAGE_SIZE;
            freeRamFrames[i]=0;
            j++;
            if(found>= np) break;

        }
    }

    //finished looping over freeramframes
    
   //setting remainder

    if(found <np)
        *npages -= found;




   

    spinlock_release(&freemem_lock);

    return firstpaddr;
    
}





paddr_t *getuserppages(unsigned long npages){
    paddr_t *frames;
    unsigned long remainder=npages;
    frames=getfreeuserpages(&remainder);
    
    if(remainder>0){
        paddr_t *freeframes=alloc_upages_from_ram(remainder);
        memcpy(frames+npages, freeframes, remainder);
        kfree(freeframes);
    }

    return frames;
} 

paddr_t *alloc_upages_from_ram(unsigned long npages){
    spinlock_acquire(&stealmem_lock);
    paddr_t firstaddr=ram_stealmem(npages);
    paddr_t *newtable=kmalloc(sizeof(paddr_t)*npages ); //array pf physical (contiguous) addresses stolen
    //placing the physical addresses stolen in array newtable
    for(unsigned long i=0;i<npages;i++){
        newtable[i]=firstaddr+PAGE_SIZE*i;
    }
    spinlock_release(&stealmem_lock);
    return newtable;
    
}

void destroy_ptable(paddr_t *ptable, long npages){
    long index, i;
    if(!isTableActive())
        return 0;
    KASSERT(ptable!=0);
    spinlock_acquire(&stealmem_lock);
    for(i=0;i<npages;i++){

        index=(long)(ptable[i]/PAGE_SIZE);
        KASSERT(nRamFrames>index);
        freeRamFrames[index]=1;   //Converts physical address at i in ptable in the corresponding index infreeRamFrames nad frees it
    }
    spinlock_release(&stealmem_lock);
}