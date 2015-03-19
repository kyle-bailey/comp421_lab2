#include "trap_handlers.h"

#include "linked_list.h"
#include "process_control_block.h"
#include "memory_management.h"

void getpid_handler(ExceptionStackFrame *frame);
void delay_handler(ExceptionStackFrame *frame);

void kernel_trap_handler(ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_KERNEL interrupt handler...\n");
  if(frame->code == YALNIX_GETPID){
    TracePrintf(1, "GetPid requested.\n");
    getpid_handler(frame);
  } else if (frame->code == YALNIX_DELAY) {
    TracePrintf(1, "Delay requested.\n");
    delay_handler(frame);
  } else if (frame->code == YALNIX_BRK) {
    TracePrintf(1, "Brk requested.\n");
    brk_handler(frame);
  }
}

void clock_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_CLOCK interrupt handler...\n");
  decrement_delays();
}

void illegal_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_ILLEGAL interrupt handler...\n");
  Halt();
}

void memory_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_MEMORY interrupt handler...\n");
  Halt();
}

void math_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_MATH interrupt handler...\n");
  Halt();
}

void tty_recieve_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_TTY_RECEIVE interrupt handler...\n");
  Halt();
}

void tty_transmit_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_TTY_TRANSMIT interrupt handler...\n");
  Halt();
}

void getpid_handler(ExceptionStackFrame *frame) {
  struct schedule_item *item = get_head();
  struct process_control_block *pcb = item->pcb;
  int pid = pcb->pid;
  frame->regs[0] = pid;
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

  move_head_to_tail();
  select_next_process();

  item = get_head();
  struct process_control_block *next_pcb = item->pcb;

  ContextSwitch(context_switch_helper, &current_pcb->saved_context, (void *)current_pcb, (void *)next_pcb);

  frame->regs[0] = 0;
  return;
}