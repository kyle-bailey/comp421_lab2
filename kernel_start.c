#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include "trap_handlers.h"
#include "process_control_block.h"
#include "memory_management.h"
#include "page_table_management.h"
#include "load_program.h"

void **interrupt_vector_table;

void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args) {
  TracePrintf(1, "KernelStart called.\n");

  int i;

  //initalize structure that keeps track of free pages
  init_is_physical_page_occupied(pmem_size);

  TracePrintf(2, "Free pages structure initialized.\n");

  //mark kernel stack as occupied
  occupy_pages_in_range((void *)KERNEL_STACK_BASE, (void *)KERNEL_STACK_LIMIT);

  //initalize the interrupt vector table
  interrupt_vector_table = malloc(TRAP_VECTOR_SIZE * sizeof(void *));
  for(i = 0; i < TRAP_VECTOR_SIZE; i++){
    switch(i){
      case TRAP_KERNEL:
        interrupt_vector_table[i] = kernel_trap_handler;
        TracePrintf(3, "%d\n", TRAP_KERNEL);
        break;
      case TRAP_CLOCK:
        interrupt_vector_table[i] = clock_trap_handler;
        TracePrintf(3, "%d\n", TRAP_CLOCK);
        break;
      case TRAP_ILLEGAL:
        interrupt_vector_table[i] = illegal_trap_handler;
        TracePrintf(3, "%d\n", TRAP_ILLEGAL);
        break;
      case TRAP_MEMORY:
        interrupt_vector_table[i] = memory_trap_handler;
        TracePrintf(3, "%d\n", TRAP_MEMORY);
        break;
      case TRAP_MATH:
        interrupt_vector_table[i] = math_trap_handler;
        TracePrintf(3, "%d\n", TRAP_MATH);
        break;
      case TRAP_TTY_RECEIVE:
        interrupt_vector_table[i] = tty_recieve_trap_handler;
        TracePrintf(3, "%d\n", TRAP_TTY_RECEIVE);
        break;
      case TRAP_TTY_TRANSMIT:
        interrupt_vector_table[i] = tty_transmit_trap_handler;
        TracePrintf(3, "%d\n", TRAP_TTY_TRANSMIT);
        break;
      default:
        interrupt_vector_table[i] = NULL;
    }
  }

  //Initialize REG_VECTOR_BASE privileged machine register to point to table
  WriteRegister(REG_VECTOR_BASE, (RCS421RegVal)interrupt_vector_table);

  TracePrintf(2, "Interrupt vector table initialized. REG_VECTOR_BASE written to.\n");

  //Kernel Page Table initialzation
  init_kernel_page_table();

  //Region 0 Page Table Initilization
  init_user_page_table();

  //Set PTR0 and PTR1 to point to physical address of starting pages
  WriteRegister(REG_PTR0, (RCS421RegVal)user_page_table);
  WriteRegister(REG_PTR1, (RCS421RegVal)kernel_page_table);

  TracePrintf(2, "Kernel and user page table pointers set.\n");

  //Enable virtual memory
  WriteRegister(REG_VM_ENABLE, 1);

  virt_mem_initialized = 1;

  TracePrintf(2, "Virtual memory enabled.\n");

  char *loadargs[1];
  loadargs[0] = NULL;

  LoadProgram("idle", loadargs, frame);
}