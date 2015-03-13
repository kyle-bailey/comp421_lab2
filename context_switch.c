#include "context_switch.h"

SavedContext *
context_switch_helper(SavedContext *ctxp, void *p1, void *p2){
  struct process_control_block *pcb1 = (struct process_control_block *)p1;
  struct process_control_block *pcb2 = (struct process_control_block *)p2;

  // set the frozen savedcontext of the process getting switched out.
  pcb1->saved_context = ctxp;

  // Change REG_PTR0 to the page table of process 2
  WriteRegister(REG_PTR0, (RCS421RegVal)pcb2->page_table);

  // Flush the TLB for region 0.
  WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)TLB_FLUSH_0);

  return pcb2->saved_context;
}

/**
 * This procedure is for use in the ContextSwitch function. It switches contexts from
 * process 1 to process 2, but before it does so, it copies process 1's kernel stack
 * over to process 2.
 */
SavedContext *
context_switch_helper_with_kernel_stack_copy(SavedContext *ctxp, void *p1, void *p2) {
  int i = 0;

  struct process_control_block *pcb1 = (struct process_control_block *)p1;
  struct process_control_block *pcb2 = (struct process_control_block *)p2;

  // copy the kernel stack of pcb1->page_table to pcb2->page_table
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

/**
 * This procedure is for use in the ContextSwitch function. All it does is initialize
 * the SavedContext for the pcb pointed to by p1. No context is actually switched.
 */
SavedContext *
initialize_saved_context(SavedContext *ctxp, void *p1, void *p2) {
  struct process_control_block *pcb1 = (struct process_control_block *)p1;

  pcb1->saved_context = ctxp;

  pcb1->saved_context;
}