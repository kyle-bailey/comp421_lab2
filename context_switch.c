#include "context_switch.h"

SavedContext *
context_switch_helper(SavedContext *ctxp, void *p1, void *p2){
  // struct process_control_block *pcb1 = (struct process_control_block *)p1;
  struct process_control_block *pcb2 = (struct process_control_block *)p2;

  return pcb2->saved_context;
}

SavedContext *
context_switch_helper_with_kernel_stack_copy(SavedContext *ctxp, void *p1, void *p2) {
  int i = 0;

  struct process_control_block *pcb1 = (struct process_control_block *)p1;
  struct process_control_block *pcb2 = (struct process_control_block *)p2;

  //copy the kernel stack of pcb1->page_table to pcb2->page_table
  struct pte **process_1_page_table = pcb1->page_table;
  struct pte **process_2_page_table = pcb2->page_table;

  for (i = 0; i < KERNEL_STACK_PAGES; i++) {
    memcpy(
      process_2_page_table[i + KERNEL_STACK_BASE/PAGESIZE], // dest
      process_1_page_table[i + KERNEL_STACK_BASE/PAGESIZE], // src
      PAGESIZE
    );
  }

  return context_switch_helper(ctxp, p1, p2);  
}