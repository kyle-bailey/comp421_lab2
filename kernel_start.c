#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include "trap_handlers.h"
#include "memory_management.h"
#include "page_table_management.h"
#include "load_program.h"
#include "context_switch.h"
#include "process_control_block.h"
#include "process_scheduling.h"

void **interrupt_vector_table;
int is_init = 1;

void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args) {
  TracePrintf(1, "kernel_start: KernelStart called.\n");

  int i;

  //initalize structure that keeps track of free pages
  init_is_physical_page_occupied(pmem_size);

  TracePrintf(2, "kernel_start: Free pages structure initialized.\n");

  //mark kernel stack as occupied
  occupy_pages_in_range((void *)KERNEL_STACK_BASE, (void *)KERNEL_STACK_LIMIT);

  //initalize the interrupt vector table
  interrupt_vector_table = malloc(TRAP_VECTOR_SIZE * sizeof(void *));
  for(i = 0; i < TRAP_VECTOR_SIZE; i++){
    switch(i){
      case TRAP_KERNEL:
        interrupt_vector_table[i] = kernel_trap_handler;
        break;
      case TRAP_CLOCK:
        interrupt_vector_table[i] = clock_trap_handler;
        break;
      case TRAP_ILLEGAL:
        interrupt_vector_table[i] = illegal_trap_handler;
        break;
      case TRAP_MEMORY:
        interrupt_vector_table[i] = memory_trap_handler;
        break;
      case TRAP_MATH:
        interrupt_vector_table[i] = math_trap_handler;
        break;
      case TRAP_TTY_RECEIVE:
        interrupt_vector_table[i] = tty_recieve_trap_handler;
        break;
      case TRAP_TTY_TRANSMIT:
        interrupt_vector_table[i] = tty_transmit_trap_handler;
        break;
      default:
        interrupt_vector_table[i] = NULL;
    }
  }

  //Initialize REG_VECTOR_BASE privileged machine register to point to table
  WriteRegister(REG_VECTOR_BASE, (RCS421RegVal)interrupt_vector_table);

  TracePrintf(2, "kernel_start: Interrupt vector table initialized. REG_VECTOR_BASE written to.\n");

  //Kernel Page Table initialzation
  init_kernel_page_table();

  //Region 0 Page Table Initilization
  //creating idle process
  struct process_control_block *idle_pcb = malloc(sizeof(struct process_control_block));
  idle_pcb->pid = 1;
  idle_pcb->page_table = malloc(PAGE_TABLE_SIZE);
  idle_pcb->delay = IDLE_DELAY;
  idle_pcb->parent_pid = ORPHAN_PARENT_PID; // this should be null, but is here for testing.
  idle_pcb->exit_status_queue = NULL;
  // TracePrintf(3, "kernel_start: will this kill? %p\n", idle_pcb->page_table[0]);
  prep_user_page_table(idle_pcb->page_table);
  add_pcb_to_schedule(idle_pcb);

  TracePrintf(2, "kernel_start: idle process pcb initialized.\n");

  //creating init process - temp moved to here so we can use malloc
  struct process_control_block *init_pcb = malloc(sizeof(struct process_control_block));
  init_pcb->pid = 0;
  init_pcb->page_table = malloc(PAGE_TABLE_SIZE);
  init_pcb->delay = 0;
  init_pcb->parent_pid = 1;
  init_pcb->exit_status_queue = NULL;
  prep_user_page_table(init_pcb->page_table);
  add_pcb_to_schedule(init_pcb);

  TracePrintf(2, "kernel_start: init process pcb initialized.\n");

  //Set PTR0 and PTR1 to point to physical address of starting pages
  WriteRegister(REG_PTR0, (RCS421RegVal)idle_pcb->page_table);
  WriteRegister(REG_PTR1, (RCS421RegVal)kernel_page_table);

  TracePrintf(2, "kernel_start: Kernel and user page table pointers set.\n");

  //Enable virtual memory
  WriteRegister(REG_VM_ENABLE, 1);

  virt_mem_initialized = 1;

  TracePrintf(2, "kernel_start: Virtual memory enabled.\n");

  //load idle process
  char *loadargs[1];
  loadargs[0] = NULL;
  LoadProgram("idle", loadargs, frame, idle_pcb->page_table);

  TracePrintf(2, "kernel_start: Idle process loaded.\n");

  ContextSwitch(idle_and_init_initialization, &idle_pcb->saved_context, (void *)idle_pcb, (void *)init_pcb);

  //Load init process
  if(is_init == 1){
    is_init = 0;
    if (cmd_args[0] == NULL) {
      LoadProgram("init", loadargs, frame, init_pcb->page_table);
    } else {
      LoadProgram(cmd_args[0], cmd_args, frame, init_pcb->page_table);
    }
  }

  TracePrintf(2, "kernel_start: Init process loaded.\n");

}