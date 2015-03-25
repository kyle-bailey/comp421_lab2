#include <comp421/hardware.h>
#include <comp421/yalnix.h>

#define ORPHAN_PARENT_PID -1

struct exit_status_node
{
  int exit_status;
  struct exit_status_node *next;
};

struct process_control_block
{
  int pid;
  struct pte *page_table;
  SavedContext saved_context;
  int delay;
  void *brk;
  void *user_stack_limit;
  struct exit_status_node *exit_status_queue;
  int parent_pid;
  int out_of_memory;
};

struct exit_status_node *get_next_child_exit_status_node(struct process_control_block *pcb);

void add_child_exit_status(struct process_control_block *pcb, int exit_status);

struct process_control_block * create_idle_process();
struct process_control_block * create_new_process(int pid, int parent_pid);
struct process_control_block * create_unprepped_process(int pid, int parent_pid);