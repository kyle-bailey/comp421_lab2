#include <comp421/hardware.h>
#include <comp421/yalnix.h>

void kernel_trap_handler(ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_KERNEL interrupt handler...\n");
  if(frame->code == YALNIX_GETPID){
    return get_head()->pcb->pid;
  }
  Halt();
}

void clock_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_CLOCK interrupt handler...\n");
  Halt();
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
