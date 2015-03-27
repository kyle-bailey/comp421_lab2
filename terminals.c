#include "terminals.h"
#include "process_scheduling.h"
#include "process_control_block.h"
#include "trap_handlers.h"

int new_line_in_buffer(int terminal);

void
init_charbuffers() {
  TracePrintf(0, "terminals: Initializing charbuffers\n");

  charbuffers = malloc(sizeof(struct charbuffer) * NUM_TERMINALS);

  int i;
  for (i = 0; i < NUM_TERMINALS; i++) {
    charbuffers[i].read = 0;
    charbuffers[i].write = 0;
  }
}

// this is for user typed input.
int
write_to_buffer_raw(int terminal, char *buf, int len) {
  int i;

  int num_written = 0;

  TracePrintf(3, "terminals: write_to_buffer_raw(%d, %s, %d)\n", terminal, buf, len);

  for (i = 0; i < len; i++) {
    // drop characters if our buffer is full.
    if (charbuffers[terminal].count != TERMINAL_MAX_LINE) {
      TracePrintf(3, "terminals: charbuffer state: %s\n", charbuffers[terminal].buffer);
      charbuffers[terminal].buffer[charbuffers[terminal].write] = buf[i];
      charbuffers[terminal].write = (charbuffers[terminal].write + 1) % TERMINAL_MAX_LINE;

      charbuffers[terminal].count++;
      num_written++;
    }
  }

  return num_written;
}

int
write_to_buffer(int terminal, char *buf, int len) {
  int i;

  // we want to block if there is someone already writing to this terminal
  // this is a while loop, because even if this process is woken up, some process could be
  // ahead in the queue who also wants to write.
  while (get_pcb_of_process_writing_to_terminal(terminal) != NULL) {
    struct schedule_item *item = get_head();
    struct process_control_block *current_pcb = item->pcb;

    // block the process
    current_pcb->is_waiting_to_write_to_terminal = terminal;

    // context switch.
    reset_time_till_switch();
    schedule_processes();
  }

  int num_written = 0;

  for (i = 0; i < len; i++) {
    // drop characters if our buffer is full.
    if (charbuffers[terminal].count != TERMINAL_MAX_LINE) {
      charbuffers[terminal].buffer[charbuffers[terminal].write] = buf[i];
      charbuffers[terminal].write = (charbuffers[terminal].write + 1) % TERMINAL_MAX_LINE;

      charbuffers[terminal].count++;
      num_written++;
    }
  }

  return num_written;
}

int
read_from_buffer(int terminal, char *buf, int len) {
  int i;
  
  struct schedule_item *item = get_head();
  struct process_control_block *current_pcb = item->pcb;

  // we want to block if there is not a new line in the terminal's buffer
  // this is a while loop, because even if this process is woken up, some process could be
  // ahead in the queue who also wants to read.
  while (!new_line_in_buffer(terminal)) {
    TracePrintf(3, "terminals: waiting to read a line from terminal %d\n", terminal);
    // block the process
    current_pcb->is_waiting_to_read_from_terminal = terminal;

    // context switch.
    reset_time_till_switch();
    schedule_processes();
  }

  // we now know there is a new line for us to read!
  int num_read = 0;
  int buffer_index = charbuffers[terminal].read;
  for (i = 0; i < len; i++) {
    if (charbuffers[terminal].count > 0) {
      buf[i] = charbuffers[terminal].buffer[buffer_index];

      buffer_index = (buffer_index + 1) % TERMINAL_MAX_LINE;
      charbuffers[terminal].count--;
      num_read++;
    } else {
      break;
    }
  }

  TracePrintf(2, "terminals: read %d characters\n", num_read);

  return num_read;
}

int
new_line_in_buffer(int terminal) {
  int i;

  int buffer_index = charbuffers[terminal].read;
  for (i = 0; i < charbuffers[terminal].count; i++) {
    TracePrintf(3, "terminals: running through buffer...: %c\n", charbuffers[terminal].buffer[buffer_index]);
    if (charbuffers[terminal].buffer[buffer_index] == '\n') {
      return 1;
    }

    buffer_index = (buffer_index + 1) % TERMINAL_MAX_LINE;
  }

  return 0;
}