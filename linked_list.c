#include "linked_list.h"

struct schedule_item *head = NULL;

struct schedule_item *
get_head() {
  return head;
}

void
move_head_to_tail() {
  if(head != NULL){
    struct schedule_item *current = head;
    struct schedule_item *new_head = head->next;
    while(current->next != NULL){
      current = current->next;
    }
    current->next = head;
    head = new_head;
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

void
add_pcb_to_schedule(struct process_control_block *pcb) {
  struct schedule_item *new_item = malloc(sizeof(struct schedule_item));
  new_item->pcb = pcb;
  new_item->next = head;
  head = new_item;
}

void
decrement_delays() {
  struct schedule_item *current = head;
  while(current != NULL){
    if(current->pcb.delay >0){
      current->pcb.delay--;
    }
    current = current->next;
  }
}