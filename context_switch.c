#include "context_switch.h"

SavedContext *
context_switch_helper(SavedContext *ctxp, void *p1, void *p2){
  // struct process_control_block *pcb1 = (struct process_control_block *)p1;
  struct process_control_block *pcb2 = (struct process_control_block *)p2;

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
idle_and_init_initialization(SavedContext *ctxp, void *p1, void *p2) {
  int i = 0;

  struct process_control_block *pcb1 = (struct process_control_block *)p1;
  struct process_control_block *pcb2 = (struct process_control_block *)p2;

  // copy the kernel stack of pcb1->page_table to pcb2->page_table
  struct pte **process_1_page_table = pcb1->page_table;
  struct pte **process_2_page_table = pcb2->page_table;

  for (i = 0; i < KERNEL_STACK_PAGES; i++) {
    unsigned int process_2_physical_page_number = acquire_free_physical_page();
    void *process_2_physical_page = (void *)(long)(process_2_physical_page_number  * PAGESIZE + PMEM_BASE);
    void *process_1_physical_page = (void *)(long)((process_1_page_table[i + KERNEL_STACK_BASE/PAGESIZE]->pfn * PAGESIZE) + PMEM_BASE);
    memcpy(
      process_2_physical_page, // dest
      process_1_physical_page, // src
      PAGESIZE
    );

    process_2_page_table[i + KERNEL_STACK_BASE/PAGESIZE]->pfn = process_2_physical_page_number;
    process_2_page_table[i + KERNEL_STACK_BASE/PAGESIZE]->valid = 1;
    process_2_page_table[i + KERNEL_STACK_BASE/PAGESIZE]->kprot = process_1_page_table[i + KERNEL_STACK_BASE/PAGESIZE]->kprot;
    process_2_page_table[i + KERNEL_STACK_BASE/PAGESIZE]->uprot = process_1_page_table[i + KERNEL_STACK_BASE/PAGESIZE]->uprot;
  }

  return pcb1->saved_context;  
}