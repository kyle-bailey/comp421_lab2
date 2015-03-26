#include "trap_handlers.h"
#include <stdio.h>
#include "process_scheduling.h"
#include "process_control_block.h"
#include "memory_management.h"
#include "context_switch.h"
#include "load_program.h"

void getpid_handler(ExceptionStackFrame *frame);
void delay_handler(ExceptionStackFrame *frame);
void exit_handler(ExceptionStackFrame *frame);
void fork_trap_handler(ExceptionStackFrame *frame);
void wait_trap_handler(ExceptionStackFrame *frame);

#define SCHEDULE_DELAY  2
int time_till_switch = SCHEDULE_DELAY;

void kernel_trap_handler(ExceptionStackFrame *frame) {
  TracePrintf(1, "trap_handlers: Entering TRAP_KERNEL interrupt handler...\n");

  int code = frame->code;

  switch (code) {
    case YALNIX_GETPID:
      TracePrintf(1, "trap_handlers: GetPid requested.\n");
      getpid_handler(frame);
      break;
    case YALNIX_DELAY:
      TracePrintf(1, "trap_handlers: Delay requested.\n");
      delay_handler(frame);
      break;
    case YALNIX_BRK:
      TracePrintf(1, "trap_handlers: Brk requested.\n");
      brk_handler(frame);
      break;
    case YALNIX_EXIT:
      TracePrintf(1, "trap_handlers: Exit requested.\n");
      exit_handler(frame);
      break;
    case YALNIX_FORK:
      TracePrintf(1, "trap_handlers: Fork requested.\n");
      fork_trap_handler(frame);
      break;
    case YALNIX_EXEC:
      TracePrintf(1, "trap_handlers: Exec requested.\n");
      exec_trap_handler(frame);
      break;
    case YALNIX_WAIT:
      TracePrintf(1, "trap_handlers: Wait requested.\n");
      wait_trap_handler(frame);
      break;
  }

}

void wait_trap_handler(ExceptionStackFrame *frame){

}

void exec_trap_handler(ExceptionStackFrame *frame){
  char *filename = frame->regs[1];
  char **argvec = frame->regs[2];

  struct schedule_item *item = get_head();

  int load_return_val = LoadProgram(filename, argvec, frame, item->pcb->page_table);
  if(load_return_val == -1){
    frame->regs[0] = ERROR;
  }
}

void fork_trap_handler(ExceptionStackFrame *frame){
  struct schedule_item *item = get_head();
  struct process_control_block *parent_pcb = item->pcb;

  //create child process
  int child_pid = get_next_pid();
  struct process_control_block *child_pcb = create_new_process(child_pid, get_current_pid());

  TracePrintf(3, "%p\n", child_pcb->page_table);

  //call specfic context switch function - copies region 0
  ContextSwitch(child_process_region_0_initialization, &parent_pcb->saved_context, (void *)parent_pcb, (void *)child_pcb);

  if (parent_pcb->out_of_memory) {
    // if this is REALLY true, then the pcb at head is child, but the page table & context are parent.
    TracePrintf(1, "trap_handlers: fork attempted, but there is not enough memory for REGION_1 copy.\n");
    struct schedule_item *current = get_head();
    TracePrintf(3, "trap_handlers: before raw_remove_head_of_schedule, \"head\" is: %d\n", current->pcb->pid);
    // Straight up remove the deformed spawn of satan at the front of the schedule.
    raw_remove_head_of_schedule();
    frame->regs[0] = ERROR;
  } else {
    //If we are the parent, return the child's PID, if we are the child return 0
    if(get_current_pid() == child_pid){
      frame->regs[0] = 0;
    } else {
      frame->regs[0] = child_pid;
    }
  }

}

void clock_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "trap_handlers: Entering TRAP_CLOCK interrupt handler...\n");
  //TESTING
  TracePrintf(1, "trap_handlers: TRAP CLOCK with PID: %d\n", get_current_pid());
  //TESTING

  time_till_switch--;
  decrement_delays();
  TracePrintf(3, "trap_handlers: time_till_switch: %d\n", time_till_switch);
  if(time_till_switch == 0) {
    TracePrintf(2, "trap_handlers: it is time to switch, we are going to schedule processes now.\n");
    reset_time_till_switch();
    schedule_processes();
  }
}

void illegal_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "trap_handlers: Entering TRAP_ILLEGAL interrupt handler...\n");

  int code = frame->code;

  int current_pid = get_current_pid();

  printf("trap_handlers: Terminating current process of pid %d due to TRAP_ILLEGAL of code %d\n", current_pid, code);

  exit_handler(frame);
}

void memory_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "trap_handlers: Entering TRAP_MEMORY interrupt handler...\n");

  int code = frame->code;

  switch (code) {
    case TRAP_MEMORY_MAPERR:
      TracePrintf(1, "trap_handlers: TRAP_MEMORY was due to: No mapping at a virtual address\n");
      break;
    case SEGV_ACCERR:
      TracePrintf(1, "trap_handlers: TRAP_MEMORY was due to: Protection violation at a virtual address\n");
      break;
    case SI_KERNEL:
      TracePrintf(1, "trap_handlers: TRAP_MEMORY was due to: Linux kernel sent SIGSEGV at virtual address\n");
      break;
    case SI_USER:
      TracePrintf(1, "trap_handlers: TRAP_MEMORY was due to: Received SIGSEGV from user\n");
      break;
  }

  Halt();
}

void math_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "trap_handlers: Entering TRAP_MATH interrupt handler...\n");

  int code = frame->code;

  int current_pid = get_current_pid();

  printf("trap_handlers: Terminating current process of pid %d due to TRAP_MATH of code %d\n", current_pid, code);

  exit_handler(frame);
}

void tty_recieve_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "trap_handlers: Entering TRAP_TTY_RECEIVE interrupt handler...\n");
  Halt();
}

void tty_transmit_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "trap_handlers: Entering TRAP_TTY_TRANSMIT interrupt handler...\n");
  Halt();
}

void getpid_handler(ExceptionStackFrame *frame) {
  frame->regs[0] = get_current_pid();
}

/*
 * Process:
 * 1. Set the delay inside the current process's pcb
 * 2. call move_head_to_tail() to move current process to the end of the schedule
 * 3. call select_next_process() to move the next process to be run to the head
 * 4. context switch from currently running process to that next process
 */
void delay_handler(ExceptionStackFrame *frame) {
  int num_ticks_to_wait = frame->regs[1];
  
  if(num_ticks_to_wait < 0){
    frame->regs[0] = ERROR;
    return;
  }

  struct schedule_item *item = get_head();
  struct process_control_block *current_pcb = item->pcb;
  current_pcb->delay = num_ticks_to_wait;

  frame->regs[0] = 0;
  if(num_ticks_to_wait > 0){
    TracePrintf(1, "trap_handlers: Delay is causing a context switch\n");
    schedule_processes();
  }
  return;
}

void exit_handler(ExceptionStackFrame *frame) {
  int exit_status = frame->regs[1];

  struct schedule_item *current = get_head();

  TracePrintf(3, "trap_handlers: Process of pid %d wants to exit\n", current->pcb->pid);

  if (!is_current_process_orphan()) {

    struct process_control_block *parent_pcb = get_pcb_by_pid(current->pcb->parent_pid);
    TracePrintf(3, "trap_handlers: parent: %d\n", parent_pcb->pid);

    add_child_exit_status(parent_pcb, exit_status);
    TracePrintf(3, "trap_handlers: Finished adding child exit status\n");
  }

  decapitate();

  TracePrintf(3, "trap_handlers: %p\n", get_head()->next);
}

void
reset_time_till_switch() {
  time_till_switch = SCHEDULE_DELAY;
}