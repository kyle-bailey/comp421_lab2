#include "memory_management.h"
#include "page_table_management.h"
#include "process_scheduling.h"
#include "process_control_block.h"

//int array that keeps track of what pages are free (0 means free, 1 means occupied)
int *is_physical_page_occupied = NULL;
void *kernel_brk = (void *)VMEM_1_BASE;
int num_physical_pages;

int virt_mem_initialized = 0;

int SetKernelBrk(void *addr) {
  int i;
  if(virt_mem_initialized) {
    int num_pages_required = ((long)UP_TO_PAGE(addr) - (long)kernel_brk)/PAGESIZE;
    if(num_free_physical_pages() < num_pages_required){
      return -1;
    } else {
      for(i = 0; i < num_pages_required; i++){
        unsigned int physical_page_number = acquire_free_physical_page();
        int vpn = ((long)kernel_brk - VMEM_1_BASE)/PAGESIZE + i;

        kernel_page_table[vpn].valid = 1;
        kernel_page_table[vpn].pfn = physical_page_number;
      }
    }
  } else {
    // SetKernelBrk should never be freeing a page we have already allocated.
    if ((long)addr <= (long)kernel_brk - PAGESIZE) {
      return -1;
    }
    occupy_kernel_pages_up_to(addr);
  }

  return 0;
}

void
brk_handler(ExceptionStackFrame *frame){
  void *addr = (void *)frame->regs[1];
  int i;

  if(UP_TO_PAGE(addr) <= MEM_INVALID_SIZE){
    frame->regs[0] = ERROR;
    return;
  }

  struct schedule_item *item = get_head();
  struct process_control_block *pcb = item->pcb;
  void *brk = pcb->brk;
  void *user_stack_limit = pcb->user_stack_limit;
  struct pte *user_page_table = pcb->page_table;

  if(UP_TO_PAGE(addr) >= DOWN_TO_PAGE(user_stack_limit) -1){
    frame->regs[0] = ERROR;
    return;
  }

  if(UP_TO_PAGE(addr) > UP_TO_PAGE(brk)){

    int num_pages_required = ((long)UP_TO_PAGE(addr) - (long)UP_TO_PAGE(brk))/PAGESIZE;
    if(num_free_physical_pages() < num_pages_required){
      frame->regs[0] = ERROR;
      return;
    } else {
      TracePrintf(3, "memory_management: preparing to acquire %d physical pages\n", num_pages_required);
      for(i = 0; i < num_pages_required; i++) {
        unsigned int physical_page_number = acquire_free_physical_page();
        int vpn = (long)UP_TO_PAGE(brk)/PAGESIZE + i;
        user_page_table[vpn].valid = 1;
        user_page_table[vpn].pfn = physical_page_number;
      }
    }
  } else if(UP_TO_PAGE(addr) < UP_TO_PAGE(brk)){
    int num_pages_to_free = ((long)UP_TO_PAGE(brk) - (long)UP_TO_PAGE(addr))/PAGESIZE;
  
    TracePrintf(3, "memory_management: preparing to free %d physical pages\n", num_pages_to_free);
    for(i = 0; i < num_pages_to_free; i++){
      user_page_table[(long)UP_TO_PAGE(brk)/PAGESIZE - i].valid = 0;
      int physical_page_number = user_page_table[(long)UP_TO_PAGE(brk)/PAGESIZE - i].pfn;
      free_physical_page(physical_page_number);
    }
  }

  frame->regs[0] = 0;
  pcb->brk = (void *)UP_TO_PAGE(addr);

  TracePrintf(1, "memory_management: brk_handler finished\n");
}

void
occupy_kernel_pages_up_to(void *end) {
  if(is_physical_page_occupied != NULL){
    occupy_pages_in_range(kernel_brk, end);
  }
  kernel_brk = (void *)UP_TO_PAGE(end);
}

void
occupy_pages_in_range(void *begin, void *end) {
  int i;
  int boundary = (long)UP_TO_PAGE(end)/PAGESIZE;
  int start = (long)DOWN_TO_PAGE(begin)/PAGESIZE;

  for(i = start; i < boundary; i++){
    is_physical_page_occupied[i] = 1;
  }
}

void
init_is_physical_page_occupied(unsigned int pmem_size) {
  //note that this will mark physical pages as occupied starting from VMEM_1_BASE up to whatever
  //is_page_occupied needs.
  num_physical_pages = pmem_size/PAGESIZE;
  is_physical_page_occupied = malloc(num_physical_pages * sizeof(int));

  memset(is_physical_page_occupied, 0, num_physical_pages);
}

int
num_free_physical_pages() {
  int count = 0;
  int i;

  for(i = 0; i < num_physical_pages; i++){
    if(is_physical_page_occupied[i] == 0){
      count++;
    }
  }

  return count;
}

void
free_physical_page(unsigned int pfn) {
  is_physical_page_occupied[pfn] = 0;
}

unsigned int
acquire_free_physical_page() {
  int i;
  for(i = 0; i < num_physical_pages; i++){
    if(is_physical_page_occupied[i] == 0){
      is_physical_page_occupied[i] = 1;
      return i;
    }
  }
  Halt();
}

unsigned int
acquire_top_physical_page() {
  unsigned int top_physical_page_addr = DOWN_TO_PAGE(VMEM_1_LIMIT - 1);

  int pfn = top_physical_page_addr/PAGESIZE;

  is_physical_page_occupied[pfn] = 1;

  return pfn;
}

void *
virt_addr_to_phys_addr(void *virt_addr) {
  void *virt_page_base = (void *)DOWN_TO_PAGE(virt_addr);
  void *phys_page_base;
  int vpn;
  int pfn;

  if (virt_page_base >= (void *)VMEM_1_BASE) {
    vpn = ((long)virt_page_base - VMEM_1_BASE)/PAGESIZE;
    pfn = kernel_page_table[vpn].pfn;
  } else {
    vpn = (long)virt_page_base/PAGESIZE;
    struct schedule_item *current = get_head();
    pfn = current->pcb->page_table[vpn].pfn;
  }

  phys_page_base = (void *) (long) (pfn * PAGESIZE);

  long offset = (long)virt_addr & PAGEOFFSET;

  return (void *)((long)phys_page_base + offset);
}

//This method assumes we are okay to grow the user stack, no seatbelts
void
grow_user_stack(void *addr, void *uncasted_pcb){
  struct process_control_block *pcb = (struct process_control_block *)uncasted_pcb;
  TracePrintf(3, "memory_management: Entering grow_user_stack with process %d, for addr %p\n", pcb->pid, addr);
  int i;

  int num_pages_required = (DOWN_TO_PAGE(pcb->user_stack_limit) - DOWN_TO_PAGE(addr))/PAGESIZE;
  TracePrintf(3, "memory_management: num_pages_required: %d\n", num_pages_required);

  for(i = 0; i < num_pages_required; i++) {
    unsigned int physical_page_number = acquire_free_physical_page();
    int vpn = (long)DOWN_TO_PAGE(pcb->user_stack_limit)/PAGESIZE - i - 1;
    pcb->page_table[vpn].valid = 1;
    pcb->page_table[vpn].pfn = physical_page_number;
  }

  pcb->user_stack_limit = (void *)DOWN_TO_PAGE(addr);

  TracePrintf(3, "memory_management: finished with grow_user_stack\n");
}