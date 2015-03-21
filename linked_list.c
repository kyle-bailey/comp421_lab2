#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "linked_list.h"
#include "process_control_block.h"
#include "context_switch.h"

struct schedule_item *head = NULL;

struct schedule_item *
get_head() {
  return head;
}

void
move_head_to_tail() {
  if(head->next == NULL){
    return;
  }
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

/* 
 * Selects the next schedule item who's pcb->delay is 0, and moves it to the head of the list
* This process assumes that if we are context switching, move_head_to_tail() has already been called
 */
void
select_next_process(){
  struct schedule_item *current = head;
  struct schedule_item *previous = NULL;

  while(current != NULL){
    struct process_control_block *pcb = current->pcb;
    if(pcb->delay == 0){
      if(previous == NULL){
        return;
      } else {
        previous->next = current->next;
        current->next = head;
        head = current;
        return;
      }
    } else {
      previous = current;
      current = current->next;
    }
  }
  Halt(); //ERROR: we iterated through all processes and they all had delays
}

/* 
 * Called when we want to context switch after the current process has been running for two clock ticks
 * Moves current process to the end of the list, selects the next process, and context switches
 */
void
schedule_processes() {
  struct schedule_item *item = get_head();
  struct process_control_block *current_pcb = item->pcb;

  move_head_to_tail();
  select_next_process();

  item = get_head();
  struct process_control_block *next_pcb = item->pcb;

  ContextSwitch(context_switch_helper, &current_pcb->saved_context, (void *)current_pcb, (void *)next_pcb);
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
    struct process_control_block *pcb = current->pcb;

    if(pcb->delay > 0){
      pcb->delay--;
    }
    current = current->next;
  }
}