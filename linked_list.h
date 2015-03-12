#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include "process_control_block.h"

struct schedule_item {
  struct schedule_item *next;
  struct process_control_block *pcb;
};

struct schedule_item * get_head();
void add_schedule_item(struct process_control_block *pcb);
void decapitate();