#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>


//gets noncontiguous free physical pages
static *paddr_t getfreeuserpages(unsigned long &npages) {
    paddr_t address
    long i, j,firstpaddr[] np = (long)npages;
    long found =0;  //it is the number of free pages found
    
    if (!isTableActive()) return 0;

    spinlock_acquire(&freemem_lock); //mutual exclusion 
   
    j=0;

    for(i =0; i<nRamFrames; i++){
        if(freeRamFrames[i]==1){
            found++; //add the nb of pages found
            //add to array every new available page
            
            firstpaddr[j]=i;
            freeRamFrames[i]=0;
            j++;
            if(found>= np) break;

        }
    }

    //finished looping over freeramframes
    
   //setting remainder

    if(found <np)
    npages -= found;


    //transfrom values in firstpaddr to paddr_t

    for(int i=0; i<found; i++)
    {
        firstpaddr[i] *= PAGE_SIZE;
    }

    spinlock_release(&freemem_lock)
    return firstpaddress;
    
}





paddr_t *getuserppages(unsigned long npages){
    paddr_t *frames;
    unsigned long reminder=npages;
    frames=getfreeuserpages(&remainder);
    
    if(reminder>0){
        paddr_t *freeframes=alloc_pages_from_ram(reminder);
        memcpy(frames+npages, freeframes, reminder);
        kfree(freeframes);
    }

    return frames;
} 

paddr_t *alloc_upages_from_ram(unsigned long npages){
    spinlock_acquire(&stealmem_lock);
    paddr_t firstaddr=ram_stealmem(npages);
    paddr_t *newtable=kmalloc(sizeof(paddr_t)*npages ); //array pf physical (contiguous) addresses stolen
    
    
    //placing the physical addresses stolen in array newtable
    for(long i=0;i<npages;i++){
        newtable[i]=firstaddr+PAGE_SIZE*i;
    }
    spinlock_release(&stealmem_lock);

    spinlock_acquire(&freemem_lock);

    //index of first physical address that has been stolen
    long firstI=(long)(firstaddr/PAGE_SIZE)

    //update freeRamFrames
    for(long i=firstI;i<nRamFrames && i<firstI+npages;i++){
        freeRamFrames[i]=(unsigned char) 0;
    }
    spinlock_release(&freemem_lock);
    return newtable;
    
}