#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "process_scheduling.h"
#include "process_control_block.h"
#include "context_switch.h"
#include "trap_handlers.h"
#include "page_table_management.h"

int can_idle_switch();
int next_pid = BASE_PID;

struct schedule_item *head = NULL;

int
get_current_pid(){
  struct schedule_item *item = get_head();
  struct process_control_block *pcb = item->pcb;
  return pcb->pid;
}

int
get_next_pid(){
  return next_pid++;
}

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
    head->next = NULL;
    current->next = head;
    head = new_head;
  }
}

int // 1 for success, 0 for failure
move_next_process_to_head(int delay) {
  struct schedule_item *current = head;
  struct schedule_item *previous = NULL;

  while(current != NULL) {
    struct process_control_block *pcb = current->pcb;
    if(pcb->delay == delay){
      if(previous == NULL){
        return 1;
      } else {
        previous->next = current->next;
        current->next = head;
        head = current;
        return 1;
      }
    } else {
      previous = current;
      current = current->next;
    }
  }

  return 0;
}

/* 
 * Selects the next schedule item who's pcb->delay is 0, and moves it to the head of the list
* This process assumes that if we are context switching, move_head_to_tail() has already been called
 */
void
select_next_process(){
  if (move_next_process_to_head(0)) {
    return;
  } else if (move_next_process_to_head(IDLE_DELAY)) {
    return;
  }

  Halt(); //ERROR: we iterated through all processes and they all had delays
}

/* 
 * Called when we want to context switch after the current process has been running for two clock ticks
 * Moves current process to the end of the list, selects the next process, and context switches
 */
void
schedule_processes() {
  TracePrintf(2, "process_scheduling: Beginning schedule_processes.\n");

  struct schedule_item *item = get_head();
  struct process_control_block *current_pcb = item->pcb;

  // if idle is at the head, make sure that there is something to switch to. If there isn't, we
  // don't need to context switch.
  if (current_pcb->pid != 1 || can_idle_switch()) { 
    move_head_to_tail();
    select_next_process();

    item = get_head();
    struct process_control_block *next_pcb = item->pcb;

    ContextSwitch(context_switch_helper, &current_pcb->saved_context, (void *)current_pcb, (void *)next_pcb);

    reset_time_till_switch();
  }
}

void
schedule_processes_during_decapitate() {
  TracePrintf(2, "process_scheduling: Beginning schedule_processes_during_decapitate.\n");

  // if idle is at the head, make sure that there is something to switch to. If there isn't, we
  // don't need to context switch.
  struct schedule_item *old_head = get_head();
  struct process_control_block *old_head_pcb = old_head->pcb;

  // we don't want to include the old head in the selection process.
  head = old_head->next;

  select_next_process();

  struct schedule_item *next_head = get_head();
  struct process_control_block *next_pcb = next_head->pcb;

  ContextSwitch(context_switch_helper, &old_head_pcb->saved_context, (void *)old_head_pcb, (void *)next_pcb);

  reset_time_till_switch();
}

void
decapitate() {
  struct schedule_item *current = get_head();
  
  if (current == NULL) {
    TracePrintf(0, "linked_list: You are trying to decapitate when there are no processes.\n");
    Halt();    
  }

  struct process_control_block *current_pcb = current->pcb;

  if (current_pcb->pid == IDLE_PID) {
    TracePrintf(0, "linked_list: You are trying to decapitate with idle as head.\n");
    Halt();
  }

  schedule_processes_during_decapitate();

  free_page_table(current->pcb->page_table);
  free(current->pcb);
  free(current);

}

void
add_pcb_to_schedule(struct process_control_block *pcb) {
  struct schedule_item *new_item = malloc(sizeof(struct schedule_item));
  new_item->pcb = pcb;
  new_item->next = head;
  head = new_item;
  TracePrintf(2, "new_item->next : %p\n", new_item->next);
  if (new_item->next != NULL) {
    TracePrintf(2, "new_item->next->next : %p\n", new_item->next->next);
  }
}

void
decrement_delays() {
  TracePrintf(2, "Starting to decrement delays.\n");
  struct schedule_item *current = head;
  while(current != NULL){
    struct process_control_block *pcb = current->pcb;

    if(pcb->delay > 0){
      pcb->delay--;
    }

    current = current->next;
  }
}

int
can_idle_switch() {
  struct schedule_item *current = head->next;
  while (current != NULL) {
    struct process_control_block *pcb = current->pcb;

    if (pcb->delay == 0) {
      return 1;
    }

    current = current->next;
  }

  return 0;
}

int
is_current_process_orphan() {
  struct schedule_item *head = get_head();

  // ORPHAN_PARENT_PID defined in process_control_block.h
  return head->pcb->parent_pid == ORPHAN_PARENT_PID;
}

struct process_control_block *
get_pcb_by_pid(int pid) {
  struct schedule_item *current = get_head();

  if (current == NULL) {
    return NULL;
  }

  while (current != NULL) {
    if (current->pcb->pid == pid) {
      return current->pcb;
    }

    current = current->next;
  }

  return NULL;
}