#include <comp421/hardware.h>
#include <comp421/yalnix.h>

//int array that keeps track of what pages are free (0 means free, 1 means not free)
int *is_page_free;
void *kernel_brk = PMEM_BASE;
void *interrupt_vector_table;

int 
SetKernelBrk(void *addr) {
  if(virt_mem_initialized){
    //more complicated stuff
    } else {
      occupy_pages_up_to(addr);
    }
  }
}

void 
KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args){

  int i;

  //initalize structure that keeps track of free pages
  is_page_free = malloc(pmem_size/PAGESIZE * sizeof(int));
  memset(is_page_free,0,sizeof(is_page_free));

  // Set all pages from PMEM_BASE up to orig_break as in use
  occupy_pages_up_to(orig_brk);

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

  WriteRegister(REG_VECTOR_BASE, (RCS421RegVal)&interrupt_vector_table);

}

void
occupy_pages_up_to(void *end) {
  int i;

  int boundary = ((long)UP_TO_PAGE(end) - (long)kernel_brk/PAGESIZE;
  for(i = 0; i < boundary; i++){
    is_page_free[i] = 1;
  }
  kernel_brk = (long)UP_TO_PAGE(end);
}