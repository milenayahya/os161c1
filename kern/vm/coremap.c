
#include <coremap.h>

//unsigned long get_stolenmem(void);

struct page_queue *head=NULL;
struct page_queue *tail=NULL;

void queue_page(vaddr_t frame, struct segment *seg){
    //If not initialized
    if(tail==NULL){
        KASSERT(head==NULL && tail==NULL);
        tail=(struct page_queue *)kmalloc(sizeof(struct page_queue));
        head=tail;
    }else{
        //create new tail            
        tail->next=(struct page_queue *)kmalloc(sizeof(struct page_queue));
        //update the tail
        tail=tail->next;
    }
    //new tail
    tail->index=((frame-(seg->vbaseaddr&PAGE_FRAME))&PAGE_FRAME)/PAGE_SIZE;
    tail->seg=seg;
    tail->next=NULL;
}
/// @brief Finds the first page loaded/swapped in, 
/// retrieves the physical address from the respective segment (stored in page_queue), 
/// swaps it out and updates the entry,
/// returns it and pops from the list.
/// @return 

paddr_t dequeue_page(){
    if(head==NULL){
        panic("No pages to dequeue");
    }
    paddr_t output=head->seg->ptable[head->index];
    KASSERT((output&PAGE_FRAME)==output);

    struct page_queue *tmp=head;
    off_t ret_offset;
    swap_out(head->seg->ptable[head->index],&ret_offset);          
    ret_offset|=PAGE_SWAPPED;

    invalidate_entry(head->index*PAGE_SIZE, output);

    head->seg->ptable[head->index]=ret_offset;
    head=head->next;
    kfree(tmp);
    return output;
}

void clear_queue(struct addrspace *as){
    struct page_queue *iterator1=NULL;
    struct page_queue *iterator2=head;
    while(iterator2!=NULL){
        if(iterator2->seg==&(as->seg1) ||
            iterator2->seg==&(as->seg2) ||
            iterator2->seg==&(as->stackseg)){
                void * tmp_pointer=(void*) iterator2;
                iterator2=iterator2->next;
                kfree(tmp_pointer);
                if(iterator1!=NULL)
                    iterator1->next=iterator2;
                else head=iterator2;
            }else{
                iterator1=iterator2;
                iterator2=iterator2->next;
            }
    }
    tail=iterator1;
    return;
}


//gets noncontiguous free physical pages
paddr_t * getfreeuserpages(unsigned long *npages, paddr_t * ptable) {
    
    long i, j, np = (long)*npages;
    long found =0;  //it is the number of free pages found

    
    if (!isTableActive()) return 0;

    spinlock_acquire(&freemem_lock); //mutual exclusion 
   
    j=0;

    for(i =0; i< nRamFrames; i++){
        if(freeRamFrames[i]==1){
            found++; //add the nb of pages found
            //add to array every new available page
            //transfrom values in firstpaddr to paddr_t
            
            ptable[j]=(paddr_t) i*PAGE_SIZE;
            freeRamFrames[i]=0;
            j++;
            if(found>= np) break;

        }
    }

    //finished looping over freeramframes
    
   //setting remainder

    if(found <=np)      //If it's less-equal to the number of pages requested we have to remove the amt found
        *npages -= found;

    spinlock_release(&freemem_lock);

    return ptable;
    
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

    frames=(paddr_t *)kmalloc(sizeof(paddr_t)*npages);

    unsigned long remainder=npages;
    frames=getfreeuserpages(&remainder, frames);
    //frames=getfreeppagesCONTIG( npages, frames); !!!!previous implementation!!!

    //getfreeuserpages changes the variable remainder so it becomes the number of pages 
    //still unallocated and requiring a ram_stealmem
    
    
    if(frames!=0 && isTableActive() && (remainder==0 || alloc_upages_from_ram(remainder, npages-remainder, frames))){
          spinlock_acquire(&freemem_lock);
          KASSERT(allocSize!=NULL);
          long index=(long) (frames[0]/PAGE_SIZE);
          KASSERT(index< nRamFrames);
          allocSize[index] = npages;
          spinlock_release(&freemem_lock);
          return frames;
    }else{
      kfree(frames);
      return 0;
    }
    
    
} 

/*
*   Stores physical addresses of stolen frames in the array ptable starting at the offset until it stores npages-offset pages
* 
*/
int alloc_upages_from_ram(unsigned long npages, unsigned long offset, paddr_t *ptable){
    unsigned long i;
    spinlock_acquire(&stealmem_lock);
    paddr_t firstaddr=ram_stealmem(npages);
    if(firstaddr==0){
      for(i = 0; i< npages; i++)                       // If ram is full free evertything
        freeRamFrames[(int)ptable[i]/PAGE_SIZE] = 1;  //loop on page table and free the corresponding freeRamframes

      spinlock_release(&stealmem_lock);             
      return 0;
    }
      
    //placing the physical addresses stolen in array newtable
    for(unsigned long i=offset; i<npages;i++){
        ptable[i]=firstaddr+PAGE_SIZE*i;
    }
    spinlock_release(&stealmem_lock);
    return 1;
}

void destroy_ptable(paddr_t *ptable, long npages){
    long index, i;
    if(!isTableActive())
        return;
    KASSERT(allocSize!=NULL);
    KASSERT(ptable!=0);
    spinlock_acquire(&freemem_lock);
    for(i=0;i<npages;i++){
        if(ptable[i]==0 || (ptable[i]&PAGE_SWAPPED)!=0)
          continue;
        index=(long)(ptable[i]/PAGE_SIZE);
        KASSERT(nRamFrames>index);
        freeRamFrames[index]=(unsigned char)1;   //Converts physical address at i in ptable in the corresponding index infreeRamFrames nad frees it
    }
    spinlock_release(&freemem_lock);

    #if OPT_SWAPPING
    for(i=0;i<npages;i++){
      if(ptable[i]&PAGE_SWAPPED){
        swap_clear((off_t)(ptable[i]&(~PAGE_SWAPPED)));
      }
    }
    #endif
    kfree(ptable);
}


//DUMB COREMAP


/*
* Contiguous page table
* 
* [Deprecated]
*
*/
paddr_t *
getfreeppagesCONTIG(unsigned long npages) {
  paddr_t *addr;	
  addr = (paddr_t *)kmalloc(sizeof(paddr_t)*(npages));
  long i, first, found, np = (long)npages;

  if (!isTableActive()) {
    kfree(addr);
    return 0; 
  }


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
      freeRamFrames[i] = (unsigned char)0;      //occupies frames and stores the relative address
      //addr[i] = (paddr_t) i*PAGE_SIZE;           //FOUND IT!!!
      addr[i-found]=(paddr_t) i*PAGE_SIZE;
    }
    allocSize[found] = np;
    
  }
  else {
    spinlock_release(&freemem_lock);
    kfree(addr);
    return 0;
  }

  spinlock_release(&freemem_lock);

  return addr;
}
