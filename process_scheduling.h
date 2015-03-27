#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>

#define INIT_PID 0
#define IDLE_PID 1
#define BASE_PID 2
#define IDLE_DELAY -1

struct schedule_item {
  struct schedule_item *next;
  struct process_control_block *pcb;
};

int get_current_pid();

int get_next_pid();

struct schedule_item * get_head();

void move_head_to_tail();

void select_next_process();

void schedule_processes();

void decapitate();

void raw_remove_head_of_schedule();

void add_pcb_to_schedule(struct process_control_block *pcb);

void decrement_delays();

int is_current_process_orphan();

struct process_control_block *get_pcb_by_pid(int pid);

void wake_up_a_reader_for_terminal(int terminal);

void wake_up_a_writer_for_terminal(int terminal);

struct process_control_block *get_pcb_of_process_writing_to_terminal(int terminal);