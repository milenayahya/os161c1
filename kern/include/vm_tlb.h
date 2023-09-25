#include <types.h>
#include <spl.h>
#include <lib.h>
#include <mips/tlb.h>
#include <vm.h>
#include <mips/tlb.h>
#include "opt-tlb_management.h"
#include "vmstats.h"
#include <cpu.h>


int tlb_get_victim(void);
int invalidate_entry(vaddr_t entryhi, paddr_t entrylow);
int insert_tlb_entry(vaddr_t faultaddress, paddr_t paddr, bool readonly);