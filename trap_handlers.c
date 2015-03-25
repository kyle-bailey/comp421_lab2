#include "trap_handlers.h"

#include "process_scheduling.h"
#include "process_control_block.h"
#include "memory_management.h"
#include "context_switch.h"

void getpid_handler(ExceptionStackFrame *frame);
void delay_handler(ExceptionStackFrame *frame);
void exit_handler(ExceptionStackFrame *frame);
void fork_trap_handler(ExceptionStackFrame *frame);

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
  }

}

void fork_trap_handler(ExceptionStackFrame *frame){
  //create child process
  int child_pid = get_next_pid();
  struct process_control_block *child_pcb = create_new_process(child_pid, get_current_pid());

  struct schedule_item *item = get_head();
  struct process_control_block *parent_pcb = item->pcb;

  //call specfic context switch function - copies region 0
  ContextSwitch(child_process_region_0_initialization, &parent_pcb->saved_context, (void *)parent_pcb, (void *)child_pcb);

  //If we are the parent, return the child's PID, if we are the child return 0
  if(get_current_pid() == child_pid){
    frame->regs[0] = 0;
  } else {
    frame->regs[0] = child_pid;
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
  Halt();
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
  Halt();
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

  if (!is_current_process_orphan()) {
    struct schedule_item *current = get_head();

    struct process_control_block *parent_pcb = get_pcb_by_pid(current->pcb->parent_pid);
    TracePrintf(3, "trap_handlers: parent_pcb: %p\n", parent_pcb);

    add_child_exit_status(parent_pcb, exit_status);
    TracePrintf(3, "trap_handlers: Finished adding child exit status\n");
  }

  decapitate();

  schedule_processes();

  TracePrintf(3, "trap_handlers: %p\n", get_head()->next);
}

void
reset_time_till_switch() {
  time_till_switch = SCHEDULE_DELAY;
}