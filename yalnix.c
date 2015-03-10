#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include <string.h>
#include "trap_handlers.h"
#include "process_control_block.h"

void occupy_kernel_pages_up_to(void *end);


//int array that keeps track of what pages are free (0 means free, 1 means occupied)
int *is_page_occupied;
int virt_mem_initialized = 0;
void *kernel_brk = (void *)VMEM_1_BASE;
void **interrupt_vector_table;
struct pte *kernel_page_table;
struct pte *user_page_table = NULL;

int SetKernelBrk(void *addr) {
  if(virt_mem_initialized) {
    //more complicated stuff
  } else {
    // SetKernelBrk should never be freeing a page we have already allocated.
    if ((long)addr <= (long)kernel_brk - PAGESIZE) {
      return -1;
    }
    occupy_kernel_pages_up_to(addr);
  }

  return 0;
}

void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args) {
  TracePrintf(1, "KernelStart called.\n");

  int i;

  //initalize structure that keeps track of free pages
  //note that this will mark physical pages as occupied starting from VMEM_1_BASE up to whatever
  //is_page_occupied needs.
  is_page_occupied = malloc(pmem_size/PAGESIZE * sizeof(int));

  memset(is_page_occupied, 0, sizeof(is_page_occupied));

  TracePrintf(2, "Free pages structure initialized.\n");

  //initalize the interrupt vector table
  interrupt_vector_table = malloc(TRAP_VECTOR_SIZE * sizeof(void *));
  for(i = 0; i < TRAP_VECTOR_SIZE; i++){
    switch(i){
      case TRAP_KERNEL:
        interrupt_vector_table[i] = &kernel_trap_handler;
        break;
      case TRAP_CLOCK:
        interrupt_vector_table[i] = &clock_trap_handler;
        break;
      case TRAP_ILLEGAL:
        interrupt_vector_table[i] = &illegal_trap_handler;
        break;
      case TRAP_MEMORY:
        interrupt_vector_table[i] = &memory_trap_handler;
        break;
      case TRAP_MATH:
        interrupt_vector_table[i] = &math_trap_handler;
        break;
      case TRAP_TTY_RECEIVE:
        interrupt_vector_table[i] = &tty_recieve_trap_handler;
        break;
      case TRAP_TTY_TRANSMIT:
        interrupt_vector_table[i] = &tty_transmit_trap_handler;
        break;
      default:
        interrupt_vector_table[i] = NULL;
    }
  }

  //Initialize REG_VECTOR_BASE privileged machine register to point to table
  WriteRegister(REG_VECTOR_BASE, (RCS421RegVal)&interrupt_vector_table);

  TracePrintf(2, "Interrupt vector table initialized. REG_VECTOR_BASE written to.\n");

  //Kernel Page Table initialzation
  kernel_page_table = malloc(PAGE_TABLE_SIZE);

  int end_of_text = ((long)&_etext - (long)VMEM_1_BASE) / PAGESIZE;
  int end_of_heap = ((long)kernel_brk - (long)VMEM_1_BASE) / PAGESIZE;

  TracePrintf(2, "End of heap: %d\n", end_of_heap);

  for(i = 0; i < PAGE_TABLE_LEN; i++){
    if(i < end_of_text){
      kernel_page_table[i].valid = 1;
      kernel_page_table[i].kprot = 5; // 101
    } else if(i <= end_of_heap) {
      kernel_page_table[i].valid = 1;
      kernel_page_table[i].kprot = 3; // 110
    } else {
      kernel_page_table[i].valid = 0;
      kernel_page_table[i].kprot = 3; // 110
    }
    kernel_page_table[i].uprot = 0; // 000
    kernel_page_table[i].pfn = i + (long)VMEM_1_BASE/PAGESIZE;
  }

  TracePrintf(2, "Kernel page table initialized.\n");

  //Region 0 Page Table Initilization
  user_page_table = malloc(PAGE_TABLE_SIZE);

  for(i = 0; i < PAGE_TABLE_LEN; i++) {
    if (i >= KERNEL_STACK_BASE / PAGESIZE) {
      user_page_table[i].valid = 1;
      user_page_table[i].kprot = 3;
      user_page_table[i].uprot = 0;
    } else {
      user_page_table[i].valid = 0;
      user_page_table[i].kprot = 0; // 000
      user_page_table[i].uprot = 6; // 110
    }
    user_page_table[i].pfn = i;
  }

  for (i = 508; i < 512; i++) {
    TracePrintf(2, "%d\n", user_page_table[i].valid);
  }

  TracePrintf(2, "User page table initialized.\n");

  //Set PTR0 and PTR1 to point to physical address of starting pages
  WriteRegister(REG_PTR0, (RCS421RegVal)user_page_table);
  WriteRegister(REG_PTR1, (RCS421RegVal)kernel_page_table);

  TracePrintf(2, "Kernel and user page table pointers set.\n");

  //Enable virtual memory
  WriteRegister(REG_VM_ENABLE, 1);

  TracePrintf(2, "Virtual memory enabled.\n");
}

void
occupy_kernel_pages_up_to(void *end) {
  int i;

  int boundary = (UP_TO_PAGE(end) - (long)kernel_brk)/PAGESIZE;
  TracePrintf(2, "Boundary: %d\n", boundary);
  for(i = 0; i < boundary; i++){
    if (is_page_occupied != NULL) {
      is_page_occupied[i+(long)kernel_brk/PAGESIZE] = 1;
    }
  }
  kernel_brk = (void *)UP_TO_PAGE(end);
  TracePrintf(2, "Up to page: %p\n", UP_TO_PAGE(end));
  TracePrintf(2, "Kernel brk: %p\n", kernel_brk);
  TracePrintf(2, "Kernel brk/PAGESIZE: %ld\n", (long)kernel_brk/PAGESIZE);
}