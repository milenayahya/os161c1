#include <pt.h>



struct page_queue *head=NULL;
struct page_queue *tail=NULL;

void queue_page(long frame){
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
    tail->index=frame;
    tail->next=NULL;
}

long dequeue_page(){
    if(head==NULL){
        return 0;
    }
    long output=head->index;
    struct page_queue *tmp=head;
    head=head->next;
    kfree(tmp);
    return output;
}



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
/// @return the address of the allocated frame, 0 in case of failure to allocate 
paddr_t getuserppage(){

    paddr_t frame = getfreeuserpage();
    if (frame == 0)
        frame=alloc_upage_from_ram();
    

    return frame;

}

//getuserppage calls getfreeuserpage which looks for an available frame among the stolen ones, if it finds one,
//it allocates it to the page. If it does not find one, alloc_upage_from_ram
//is called and it steals a new frame from the RAM. It returns the physical address of the new frame,
//or it returns 0 in case of failure (and soon to be implemented: finds a victim,i.e, page replacement)

paddr_t getfreeuserpage(){ //looks for an available frame and allocates it to a page

    if (!isTableActive()) return 0;
    spinlock_acquire(&freemem_lock); //mutual exclusion 
    unsigned long i;

    for(i =0; i<(get_stolenmem()); i++){
        if(freeRamFrames[i]==1){
            
            freeRamFrames[i]=0;
            spinlock_release(&freemem_lock);
            return (paddr_t) i*PAGE_SIZE;
    
        }
    }
    spinlock_release(&freemem_lock);
    return 0;
}

paddr_t alloc_upage_from_ram(){
    spinlock_acquire(&stealmem_lock);
    paddr_t paddr=ram_stealmem(1);
    if(paddr==0){
      // implement page replacement
        
      spinlock_release(&stealmem_lock);             
      return paddr;
    }
    //in case of success
    spinlock_release(&stealmem_lock);
    return paddr;
}

