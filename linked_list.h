#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>

struct schedule_item {
  struct schedule_item *next;
  struct process_control_block *pcb;
};

struct schedule_item * get_head();
void move_head_to_tail();
void select_next_process();
void schedule_processes();
void decapitate();
void add_pcb_to_schedule(struct process_control_block *pcb);
void decrement_delays();