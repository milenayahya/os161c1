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



// paddr_t *getuserppages(unsigned long npages){
//     paddr_t *frames;
//     unsigned long remainder=npages;
//     frames=getfreeuserpages(&remainder);
//     //getfreeuserpages changes the variable remainder so it becomes the number of pages 
//     //still unallocated and requiring a ram_stealmem
    
//     if(remainder>0){
//         paddr_t *freeframes=alloc_upages_from_ram(npages);
//         memcpy(frames+npages, freeframes, remainder);
//         kfree(freeframes);
//     }

//     return frames;
// } 

paddr_t *getuserppages(unsigned long npages){
    paddr_t *frames;
    //unsigned long remainder=npages;
    //frames=getfreeuserpages(&remainder);
    frames=getfreeppagesCONTIG( npages);
    //getfreeuserpages changes the variable remainder so it becomes the number of pages 
    //still unallocated and requiring a ram_stealmem
    
    if(frames==0){
        frames=(paddr_t *)kmalloc(sizeof(paddr_t)*npages);
        alloc_upages_from_ram(npages, 0, frames);       
    }

    return frames;
} 

/*
*   Stores physical addresses of stolen frames in the array ptable starting at the offset until it stores npages-offset pages
*
*/
void alloc_upages_from_ram(unsigned long npages, unsigned long offset, paddr_t *ptable){
    spinlock_acquire(&stealmem_lock);
    paddr_t firstaddr=ram_stealmem(npages);
    KASSERT(firstaddr!=0);
    //placing the physical addresses stolen in array newtable
    for(unsigned long i=offset; i<npages;i++){
        ptable[i]=firstaddr+PAGE_SIZE*i;
    }
    spinlock_release(&stealmem_lock);
}

void destroy_ptable(paddr_t *ptable, long npages){
    long index, i;
    if(!isTableActive())
        return;
    KASSERT(ptable!=0);
    spinlock_acquire(&stealmem_lock);
    for(i=0;i<npages;i++){

        index=(long)(ptable[i]/PAGE_SIZE);
        KASSERT(nRamFrames>index);
        freeRamFrames[index]=1;   //Converts physical address at i in ptable in the corresponding index infreeRamFrames nad frees it
    }
    spinlock_release(&stealmem_lock);
    kfree(ptable);
}


//DUMB COREMAP


//Ccontiguous page table
paddr_t *
getfreeppagesCONTIG(unsigned long npages) {
  paddr_t *addr;	
  addr = (paddr_t *)kmalloc(sizeof(long)*(npages));
  long i, first, found, np = (long)npages;

  if (!isTableActive()) return 0; 
  spinlock_acquire(&freemem_lock);
  for (i=0,first=found=-1; i<nRamFrames; i++) {
    if (freeRamFrames[i]) {
      if (i==0 || !freeRamFrames[i-1]) 
        first = i; /* set first free in an interval */   
      if (i-first+1 >= np) {
        found = first;
        break;
      }
    }
  }
	
  if (found>=0) {
    for (i=found; i<found+np; i++) {
      freeRamFrames[i] = (unsigned char)0;
      addr[i] = (paddr_t) i*PAGE_SIZE;
    }
    //allocSize[found] = np;
    
  }
  else {
    addr = 0;
  }

  spinlock_release(&freemem_lock);

  return addr;
}