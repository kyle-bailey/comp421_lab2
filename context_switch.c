#include "context_switch.h"
#include "process_control_block.h"

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
  int j = 0;

  struct process_control_block *pcb1 = (struct process_control_block *)p1;
  struct process_control_block *pcb2 = (struct process_control_block *)p2;

  // copy the kernel stack of pcb1->page_table to pcb2->page_table
  struct pte *process_1_page_table = pcb1->page_table;
  struct pte *process_2_page_table = pcb2->page_table;

  TracePrintf(3, "context_switch: beginning kernel stack copy.\n");

  for (i = 0; i < KERNEL_STACK_PAGES; i++) {
    unsigned int process_2_physical_page_number = acquire_free_physical_page();

    for (j = MEM_INVALID_PAGES; j < KERNEL_STACK_BASE; j++) {
      // find the first available page from the user's heap.
      if (process_1_page_table[j].valid == 0) {
        // temporarily map that page to a physical page.
        process_1_page_table[j].valid = 1;
        process_1_page_table[j].kprot = PROT_READ | PROT_WRITE;
        process_1_page_table[j].uprot = PROT_READ | PROT_EXEC;
        process_1_page_table[j].pfn = process_2_physical_page_number;

        void *process_1_virtual_addr = (void *)(long)(((KERNEL_STACK_BASE/PAGESIZE + i) * PAGESIZE) + PMEM_BASE);
        void *temp_virt_addr_for_kernel_stack = (void *)(long)((j * PAGESIZE) + PMEM_BASE);

        WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) temp_virt_addr_for_kernel_stack);

        TracePrintf(4, "context_switch: %p\n", process_1_virtual_addr);
        TracePrintf(4, "context_switch: %p\n", temp_virt_addr_for_kernel_stack);

        memcpy(
          temp_virt_addr_for_kernel_stack, // dest
          process_1_virtual_addr, // src
          PAGESIZE
        );

        // pretend that the temp memory never existed.
        process_1_page_table[j].valid = 0;
        WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) temp_virt_addr_for_kernel_stack);

        // give the pfn from the temp memory to process 2's page table.
        process_2_page_table[i + KERNEL_STACK_BASE/PAGESIZE].valid = 1;
        process_2_page_table[i + KERNEL_STACK_BASE/PAGESIZE].kprot = process_1_page_table[i + KERNEL_STACK_BASE/PAGESIZE].kprot;
        process_2_page_table[i + KERNEL_STACK_BASE/PAGESIZE].uprot = process_1_page_table[i + KERNEL_STACK_BASE/PAGESIZE].uprot;
        process_2_page_table[i + KERNEL_STACK_BASE/PAGESIZE].pfn = process_2_physical_page_number;
        break;
      }
    }

    TracePrintf(4, "context_switch: 4Entering %d\n", i);
  }

  TracePrintf(1, "context_switch: idle_and_init_initialization completed.\n");

  return pcb1->saved_context;  
}