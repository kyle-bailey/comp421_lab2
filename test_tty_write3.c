#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>
#include "process_control_block.h"
#include "process_scheduling.h"

int
main() {
  printf("Test tty_write3 Process Initialized.\n");
  int return_val;

  char *buf = malloc(sizeof(char) * 4);
  buf[0] = 'f';
  buf[1] = 'u';
  buf[2] = 'c';
  buf[3] = 'k';

  int fork_return = Fork();

  if(fork_return == 0){
    struct scheduling_item *item = get_head();
    struct process_control_block *pcb = item->pcb;
    pcb->is_writing_to_terminal = 0;
    Delay(6);
    pcb->is_writing_to_terminal = -1;
    return_val = TtyWrite(0, buf, 5);
  } else {
    return_val = TtyWrite(0, buf, 5);
  }

  printf("Process with fork_return: %d wrote to terminal 0 and got return value: %d\n", fork_return, return_val);


  return 0;
}