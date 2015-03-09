#include <comp421/hardware.h>
#include <comp421/yalnix.h>

void kernel_trap_handler(ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_KERNEL interrupt handler...");
  Halt();
}

void clock_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_CLOCK interrupt handler...");
  Halt();
}

void illegal_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_ILLEGAL interrupt handler...");
  Halt();
}

void memory_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_MEMORY interrupt handler...");
  Halt();
}

void math_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_MATH interrupt handler...");
  Halt();
}

void tty_recieve_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_TTY_RECEIVE interrupt handler...");
  Halt();
}

void tty_transmit_trap_handler (ExceptionStackFrame *frame) {
  TracePrintf(1, "Entering TRAP_TTY_TRANSMIT interrupt handler...");
  Halt();
}
