 #include <types.h>
#include <mips/tlb.h>
#include <vm.h>

int tlb_get_victim(void);
int invalidate_entry(vaddr_t entryhi, paddr_t entrylow);