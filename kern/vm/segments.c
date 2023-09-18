#include <segments.h>


int
load_page(struct segment *seg,
	     vaddr_t vaddr, paddr_t paddr
	     int is_executable)
{
	struct iovec iov;
	struct uio u;
	int result;
	off_t file_offset;

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
		resid=(PAGE_SIZE-(vbaseoffset) > seg->filesize) ? ps->filesize : PAGE_SIZE-vbaseoffset; //how much to read from page		
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
		resid=seg->filesize-(page_index*PAGE_SIZE) -vbaseoffset;
		if(resid>PAGE_SIZE)
			resid=PAGE_SIZE;
		else if(resid<0){
			resid=0;
			file_offset=seg->filesize;
		}
			
	}
	//How much we have to read in the first page
	
	/*iov.iov_ubase = (userptr_t)vaddr;           //Destination
	iov.iov_len = PAGE_SIZE;		 			// length of the memory space
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;                   
	u.uio_resid = PAGE_SIZE;          			//amount to read from the file (Source)
//	u.uio_offset = (off_t) vaddr;        		//where we read it from in elf (vaddr) is the same vaddr we give it in the address-space of the process   
		u.uio_offset= offset + (vaddr - vbase);
	u.uio_segflg = is_executable ? UIO_USERISPACE : UIO_USERSPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = as;
*/

	uio_kinit(&iov,&u,(void *)PADDR_TO_KVADDR(dest_addr),resid,file_offset, UIO_READ);
	//PAGE_SIZE needs to be replaced by a calculated variable once we distinguish
	//between first/middle/last page.
	
	result = VOP_READ(v, &u);
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
