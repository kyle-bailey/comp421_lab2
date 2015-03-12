#include "context_switch.h"

SavedContext * context_switch_helper(SavedContext *ctxp, void *p1, void *p2){
  struct process_control_block *pcb1 = (struct process_control_block *)p1;
  struct process_control_block *pcb2 = (struct process_control_block *)p2;

  if((pcb1->pid == 0 && pcb2->pid == 1) && ){
    //copy the kernel stack of pcb1->pte to
  }
}