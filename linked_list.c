#include "linked_list.h"

struct schedule_item *head = NULL;

struct schedule_item *
get_head() {
  return head;
}

void
add_schedule_item(struct process_control_block *pcb) {

  struct schedule_item *new_item = malloc(sizeof(struct schedule_item));
  new_item->pcb = pcb;
  new_item->next = NULL;

  if(head == NULL){
    head = new_item;
  } else {
    struct schedule_item *current = head;
    while(current->next != NULL){
      current = current->next;
    }
    current->next = new_item;
  }
}

void
decapitate() {
  if(head != NULL){
    struct schedule_item *temp = head;
    head = head->next;
    free(temp->pcb);
    free(temp);
  }
}