#include "process_control_block.h"
#include "stddef.h"
#include "stdlib.h"

struct exit_status_node *pop_next_child_exit_status_node(struct process_control_block *pcb) {
  struct exit_status_node *head = pcb->exit_status_queue;

  if (head == NULL) {
    return NULL;
  } else {
    pcb->exit_status_queue = head->next;

    return head;
  }

}

void add_child_exit_status(struct process_control_block *pcb, int exit_status) {
  struct exit_status_node *current = pcb->exit_status_queue;

  struct exit_status_node *new_exit_status_node = malloc(sizeof(struct exit_status_node));
  new_exit_status_node->exit_status = exit_status;
  new_exit_status_node->next = NULL;  

  if (current == NULL) {
    pcb->exit_status_queue = new_exit_status_node;
  } else {
    while (current->next != NULL) {
      current = current->next;
    }

    current->next = new_exit_status_node;
  }
}