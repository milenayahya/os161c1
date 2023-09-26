

#include <segments.h>
#include <vmstats.h>

static void zero_a_region(paddr_t paddr, size_t n)
{
    bzero((void *)PADDR_TO_KVADDR(paddr), n);
}

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	bzero(as,sizeof(struct addrspace));

	if (as==NULL) {
		return NULL;
	}
	#if OPT_PAGING

	/*as->as_vbase1 = 0;
	as->as_ptable1 = 0;
	as->as_npages1 = 0;

	as->as_vbase2 = 0;
	as->as_ptable2 = 0;
	as->as_npages2 = 0;

	as->as_stackpbase = 0;*/
	as->initialized=0;
	#if OPT_ON_DEMAND
	// as->seg_header1=NULL;
	
	seg_init(&as->seg1);
	seg_init(&as->seg2);
	// as->seg_header2=NULL;
	#endif
	#else

	as->as_vbase1 = 0;
	as->as_pbase1 = 0;
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	as->as_pbase2 = 0;
	as->as_npages2 = 0;
	as->as_stackpbase = 0;

	

	#endif

	return as;
}

void seg_init(struct segment *seg){
	seg->vbaseaddr=0;
	seg->npages=0;
	seg->ptable=0;
	seg->filesize=0;
	seg->memsize=0;
	seg->offset=0;
	seg->elf_node=0;
	seg->flags=0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	KASSERT(as->stackseg.ptable != 0);

	as->stackseg.vbaseaddr=USERSTACK-MV_VM_STACKPAGES*PAGE_SIZE;
	*stackptr = USERSTACK;
	return 0;
}


int as_define_segment(struct addrspace *as, vaddr_t vbase, off_t offset, size_t memsize, size_t filesize, struct vnode* elf_node, uint32_t flags){
	if(as->initialized==0){
		as->seg1.vbaseaddr=vbase;
		as->seg1.offset=offset;
		as->seg1.memsize=memsize;
		as->seg1.filesize=filesize;
		as->seg1.elf_node=elf_node;
		as->seg1.flags=flags;
		as->initialized++;
		return 0;
	}else if(as->initialized==1){
		as->seg2.vbaseaddr=vbase;
		as->seg2.offset=offset;
		as->seg2.memsize=memsize;
		as->seg2.filesize=filesize;
		as->seg2.elf_node=elf_node;
		as->seg2.flags=flags;
		as->initialized++;
		return 0;
	}
	else
		return 1;
}


int
load_page(struct segment *seg,
	     vaddr_t vaddr, paddr_t paddr,
	     int is_executable)
{
	struct iovec iov;
	struct uio u;
	int result;
	(void)is_executable;

	// if (filesize > memsize) {
	// 	kprintf("ELF: warning: segment filesize > segment memsize\n");
	// 	filesize = memsize;
	// }

	// DEBUG(DB_EXEC, "ELF: Loading %lu bytes to 0x%lx\n",
	//       (unsigned long) filesize, (unsigned long) vaddr);
	unsigned long page_index=(vaddr-(seg->vbaseaddr &PAGE_FRAME))/PAGE_SIZE;
	vaddr_t vbaseoffset=(page_index==0)?seg->vbaseaddr &(~PAGE_FRAME) : 0; 									//How much to read in the last page from page start or where to start reading in the first page;
	vaddr_t dest_addr=paddr;
	size_t resid=PAGE_SIZE;
	off_t file_offset;
	vaddr_t voffset;
	
	if(page_index==0)  //first page
	{
		dest_addr=paddr+vbaseoffset;
		resid=(PAGE_SIZE-(vbaseoffset) > seg->filesize) ? seg->filesize : PAGE_SIZE-vbaseoffset; //how much to read from page		
		file_offset = seg->offset;

	}

	else if(page_index== (seg->npages)-1){  //last page
		voffset = (seg->npages-1)*PAGE_SIZE -vbaseoffset;
		dest_addr = paddr;
		file_offset= seg->offset + voffset;
		
		if(seg->filesize > voffset)
			resid = seg->filesize - voffset;
		else
		{
			resid = 0;
			file_offset= seg->filesize;
		}


	}else{  //middle page
		file_offset= seg->offset + (page_index*PAGE_SIZE) - vbaseoffset;
		if(seg->filesize<(page_index*PAGE_SIZE) -vbaseoffset){
			resid=0;  //memsize > filesize
			file_offset=seg->filesize;
		}else{
			resid=seg->filesize-(page_index*PAGE_SIZE) -vbaseoffset;
			if(resid>PAGE_SIZE)
				resid=PAGE_SIZE;
		}	
	}

	KASSERT(dest_addr-paddr<PAGE_SIZE);
	KASSERT(resid<=PAGE_SIZE);
	KASSERT(file_offset - seg->offset<=seg->filesize);
	//How much we have to read in the first page


	//zero the entire page:
	zero_a_region(paddr,PAGE_SIZE);


	if(resid != 0)
	{
		page_faults_elf++;
		page_faults_disk++;
	
	}
	else{

		page_faults_zeroed++; 
	}

	uio_kinit(&iov,&u,(void *)PADDR_TO_KVADDR(dest_addr),resid,file_offset, UIO_READ);
	
	result = VOP_READ(seg->elf_node, &u);
	if (result) {
		return result;
	}

	if (u.uio_resid != 0) {
		/* short read; problem with executable? */
		kprintf("ELF: short read on segment - file truncated?\n");
		return ENOEXEC;
	}

	/*
	 * If memsize > filesize, the remaining space should be
	 * zero-filled. There is no need to do this explicitly,
	 * because the VM system should provide pages that do not
	 * contain other processes' data, i.e., are already zeroed.
	 *
	 * During development of your VM system, it may have bugs that
	 * cause it to (maybe only sometimes) not provide zero-filled
	 * pages, which can cause user programs to fail in strange
	 * ways. Explicitly zeroing program BSS may help identify such
	 * bugs, so the following disabled code is provided as a
	 * diagnostic tool. Note that it must be disabled again before
	 * you submit your code for grading.
	 */
#if 0
	{
		size_t fillamt;

		fillamt = memsize - filesize;
		if (fillamt > 0) {
			DEBUG(DB_EXEC, "ELF: Zero-filling %lu more bytes\n",
			      (unsigned long) fillamt);
			u.uio_resid += fillamt;
			result = uiomovezeros(fillamt, &u);
		}
	}
#endif

	return result;
}

/*
 * Load an ELF executable user program into the current address space.
 *
 * Returns the entry point (initial PC) for the program in ENTRYPOINT.
 */
