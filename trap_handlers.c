#include "trap_handlers.h"
#include <stdio.h>
#include "process_scheduling.h"
#include "process_control_block.h"
#include "memory_management.h"
#include "context_switch.h"
#include "load_program.h"
#include "terminals.h"

void getpid_handler(ExceptionStackFrame *frame);
void delay_handler(ExceptionStackFrame *frame);
void exit_handler(ExceptionStackFrame *frame, int error);
void fork_trap_handler(ExceptionStackFrame *frame);
void wait_trap_handler(ExceptionStackFrame *frame);
void exec_trap_handler(ExceptionStackFrame *frame);
void tty_read_handler(ExceptionStackFrame *frame);
void tty_write_handler(ExceptionStackFrame *frame);

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
      exit_handler(frame, 0);
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
    case YALNIX_TTY_READ:
      TracePrintf(1, "trap_handlers: Tty Read requested.\n");
      tty_read_handler(frame);
      break;
    case YALNIX_TTY_WRITE:
      TracePrintf(1, "trap_handlers: Tty Write requested.\n");
      tty_write_handler(frame);
      break;
  }

}

void wait_trap_handler(ExceptionStackFrame *frame){
  int *status_ptr = (int *)frame->regs[1];

  struct schedule_item *item = get_head();
  struct process_control_block *parent_pcb = item->pcb;

  struct exit_status_node *esn = pop_next_child_exit_status_node(parent_pcb);
  if(esn == NULL){
    if(parent_pcb->num_children == 0){
      frame->regs[0] = (long)NULL;
      return;
    }
    parent_pcb->is_waiting = 1;
    reset_time_till_switch();
    schedule_processes();
    esn = pop_next_child_exit_status_node(parent_pcb);
  }

  *status_ptr = esn->exit_status;
  frame->regs[0] = esn->pid;
}

void exec_trap_handler(ExceptionStackFrame *frame){
  char *filename = (char *)frame->regs[1];
  char **argvec = (char **)frame->regs[2];

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
      parent_pcb->num_children++;
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

  exit_handler(frame, 1);
}

void memory_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "trap_handlers: Entering TRAP_MEMORY interrupt handler...\n");
  struct schedule_item *item = get_head();
  struct process_control_block *pcb = item->pcb;

  int code = frame->code;
  void *addr = frame->addr;
  void *user_stack_limit = pcb->user_stack_limit;
  void *brk = pcb->brk;

  TracePrintf(2, "trap_handlers: TRAP_MEMORY with addr: %p\n", addr);

  if(DOWN_TO_PAGE(addr) < DOWN_TO_PAGE(user_stack_limit)){
    if(UP_TO_PAGE(addr) > (UP_TO_PAGE(brk) + PAGESIZE)){
      grow_user_stack(addr, pcb);
      return;
    }
  }

  switch (code) {
    case TRAP_MEMORY_MAPERR:
      printf("TRAP_MEMORY was due to: No mapping at a virtual address\n");
      break;
    case SEGV_ACCERR:
      printf("TRAP_MEMORY was due to: Protection violation at a virtual address\n");
      break;
    case SI_KERNEL:
      printf("TRAP_MEMORY was due to: Linux kernel sent SIGSEGV at virtual address\n");
      break;
    case SI_USER:
      printf("TRAP_MEMORY was due to: Received SIGSEGV from user\n");
      break;
  }

  exit_handler(frame, 1);
}

void math_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "trap_handlers: Entering TRAP_MATH interrupt handler...\n");

  int code = frame->code;

  int current_pid = get_current_pid();

  printf("trap_handlers: Terminating current process of pid %d due to TRAP_MATH of code %d\n", current_pid, code);

  exit_handler(frame, 1);
}

void tty_recieve_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "trap_handlers: Entering TRAP_TTY_RECEIVE interrupt handler...\n");

  int terminal = frame->code;  
  char *receivedchars = malloc(sizeof(char) * TERMINAL_MAX_LINE);

  int num_received_chars = TtyReceive(terminal, receivedchars, TERMINAL_MAX_LINE);

  write_to_buffer_raw(terminal, receivedchars, num_received_chars);

  if (new_line_in_buffer(terminal)) {
    TracePrintf(3, "trap_handlers: there is a new line in buffer, so we are waking up a reader!\n");
    wake_up_a_reader_for_terminal(terminal);
  }

  TracePrintf(1, "trap_handlers: Received %d chars from terminal %d.\n", num_received_chars, terminal);
}

void tty_transmit_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "trap_handlers: Entering TRAP_TTY_TRANSMIT interrupt handler...\n");

  int terminal = frame->code;  

  struct process_control_block *done_writing_pcb = get_pcb_of_process_writing_to_terminal(terminal);

  // reset its status.
  done_writing_pcb->is_writing_to_terminal = -1;

  wake_up_a_writer_for_terminal(terminal);

  TracePrintf(1, "trap_handlers\n");
}

void
tty_read_handler(ExceptionStackFrame *frame) {
  if(terminal < 0 || terminal > NUM_TERMINALS){
    frame->regs[0] = ERROR;
    return;
  }
  int terminal = frame->regs[1];
  void *buf = (void *)frame->regs[2];
  int len = frame->regs[3];

  int num_read = read_from_buffer(terminal, buf, len);

  if (num_read >= 0) {
    frame->regs[0] = num_read;
  } else {
    frame->regs[0] = ERROR;
  }
}

void
tty_write_handler(ExceptionStackFrame *frame) {
  if(terminal < 0 || terminal > NUM_TERMINALS){
    frame->regs[0] = ERROR;
    return;
  }
  int terminal = frame->regs[1];
  void *buf = (void *)frame->regs[2];
  int len = frame->regs[3];

  // this call blocks the process if someone is already writing to terminal.
  int num_written = write_to_buffer(terminal, buf, len);

  TtyTransmit(terminal, buf, num_written);

  // now that we are waiting for the io to finish, mark it as writing.
  struct schedule_item *item = get_head();
  struct process_control_block *current_pcb = item->pcb;
  current_pcb->is_writing_to_terminal = terminal;

  if (num_written >= 0) {
    frame->regs[0] = num_written;
  } else {
    frame->regs[0] = ERROR;
  }
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

void exit_handler(ExceptionStackFrame *frame, int error) {
  int exit_status;
  if (error) {
    exit_status = ERROR;
  } else {
    exit_status = frame->regs[1];
  }

  struct schedule_item *current = get_head();

  TracePrintf(3, "trap_handlers: Process of pid %d wants to exit\n", current->pcb->pid);

  if (!is_current_process_orphan()) {

    struct process_control_block *parent_pcb = get_pcb_by_pid(current->pcb->parent_pid);
    TracePrintf(3, "trap_handlers: parent: %d\n", parent_pcb->pid);

    parent_pcb->is_waiting = 0;
    add_child_exit_status(parent_pcb, exit_status, get_current_pid());
    TracePrintf(3, "trap_handlers: Finished adding child exit status\n");
  }

  decapitate();

  TracePrintf(3, "trap_handlers: %p\n", get_head()->next);
}

void
reset_time_till_switch() {
  time_till_switch = SCHEDULE_DELAY;
}