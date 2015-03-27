#include "process_control_block.h"
#include "process_scheduling.h"
#include "page_table_management.h"
#include "stddef.h"
#include "stdlib.h"

struct exit_status_node *pop_next_child_exit_status_node(struct process_control_block *pcb) {
  struct exit_status_node *head = pcb->exit_status_queue;

  if (head == NULL) {
    return NULL;
  } else {
    pcb->exit_status_queue = head->next;
    pcb->num_children--;

    return head;
  }

}

void add_child_exit_status(struct process_control_block *parent_pcb, int exit_status, int child_pid) {
  struct exit_status_node *current = parent_pcb->exit_status_queue;

  struct exit_status_node *new_exit_status_node = malloc(sizeof(struct exit_status_node));
  new_exit_status_node->exit_status = exit_status;
  new_exit_status_node->pid = child_pid;
  new_exit_status_node->next = NULL;

  if (current == NULL) {
    parent_pcb->exit_status_queue = new_exit_status_node;
  } else {
    while (current->next != NULL) {
      current = current->next;
    }

    current->next = new_exit_status_node;
  }
}

struct process_control_block * 
create_idle_process(){
  struct process_control_block *pcb = create_unprepped_process(IDLE_PID, ORPHAN_PARENT_PID);
  prep_initial_page_table(pcb->page_table);
  return pcb;
}

struct process_control_block * 
create_new_process(int pid, int parent_pid){
  struct process_control_block *pcb = create_unprepped_process(pid, parent_pid);
  prep_page_table(pcb->page_table); 
  return pcb;
}

struct process_control_block * 
create_unprepped_process(int pid, int parent_pid){
  struct process_control_block *new_pcb = malloc(sizeof(struct process_control_block));
  new_pcb->pid = pid;
  new_pcb->page_table = create_page_table();
  new_pcb->delay = 0;
  new_pcb->parent_pid = parent_pid;
  new_pcb->exit_status_queue = NULL;
  new_pcb->out_of_memory = 0;
  new_pcb->is_waiting = 0;
  new_pcb->num_children = 0;
  new_pcb->is_waiting_to_read_from_terminal = -1;
  new_pcb->is_writing_to_terminal = -1;
  new_pcb->is_waiting_to_write_to_terminal = -1;
  add_pcb_to_schedule(new_pcb);
  return new_pcb;
}