#include "context_switch.h"
#include "process_control_block.h"
#include "memory_management.h"
#include "page_table_management.h"

SavedContext *
context_switch_helper(SavedContext *ctxp, void *p1, void *p2){
  TracePrintf(3, "context_switch: Beginning context_switch_helper.\n");
  // struct process_control_block *pcb1 = (struct process_control_block *)p1;
  struct process_control_block *pcb2 = (struct process_control_block *)p2;

  // Change REG_PTR0 to the page table of process 2
  WriteRegister(REG_PTR0, (RCS421RegVal)pcb2->page_table);

  // Flush the TLB for region 0.
  WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)TLB_FLUSH_0);

  TracePrintf(3, "context_switch: pcb2->page_table: %p\n", pcb2->page_table);
  TracePrintf(3, "context_switch: &pcb2->saved_context: %p\n", &pcb2->saved_context);
  return &pcb2->saved_context;
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

    for (j = MEM_INVALID_PAGES; j < KERNEL_STACK_BASE/PAGESIZE; j++) {
      // find the first available page from the user's heap.
      if (process_1_page_table[j].valid == 0) {
        // temporarily map that page to a physical page.
        process_1_page_table[j].valid = 1;
        process_1_page_table[j].kprot = PROT_READ | PROT_WRITE;
        process_1_page_table[j].uprot = PROT_READ | PROT_EXEC;
        process_1_page_table[j].pfn = process_2_physical_page_number;

        void *process_1_virtual_addr = (void *)(long)(((KERNEL_STACK_BASE/PAGESIZE + i) * PAGESIZE) + VMEM_0_BASE);
        void *temp_virt_addr_for_kernel_stack = (void *)(long)((j * PAGESIZE) + VMEM_0_BASE);

        WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) temp_virt_addr_for_kernel_stack);

        // TracePrintf(3, "context_switch: src: %p, dest: %p\n", process_1_virtual_addr, temp_virt_addr_for_kernel_stack);

        // copy kernel stack page into our new page of memory.
        memcpy(
          temp_virt_addr_for_kernel_stack, // dest
          process_1_virtual_addr, // src
          PAGESIZE
        );

        // pretend that the temp memory never existed.
        process_1_page_table[j].valid = 0;
        WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) temp_virt_addr_for_kernel_stack);

        // give the pfn from the temp memory to process 2's page table.
        process_2_page_table[i + KERNEL_STACK_BASE/PAGESIZE].pfn = process_2_physical_page_number;
        break;
      }
    }
  }

  WriteRegister(REG_PTR0, (RCS421RegVal)process_2_page_table);
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

  TracePrintf(1, "context_switch: idle_and_init_initialization completed.\n");

  return &pcb1->saved_context;  
}

SavedContext *
child_process_region_0_initialization(SavedContext *ctxp, void *p1, void *p2) {
  TracePrintf(3, "context_switch: Starting child_process_region_0_initialization()\n");

  int i = 0;
  int first_invalid_region_0_page = -1;

  struct process_control_block *parent_pcb = (struct process_control_block *)p1;
  struct process_control_block *child_pcb = (struct process_control_block *)p2;

  struct pte *parent_page_table = parent_pcb->page_table;
  struct pte *child_page_table = child_pcb->page_table;

  int num_user_pages = (VMEM_0_LIMIT - VMEM_0_BASE)/PAGESIZE;

  TracePrintf(3, "context_switch: MEM_INVALID_PAGES: %d\n", MEM_INVALID_PAGES);

  /*
   * Find the location of the first invalid parent region 0 page, if there is one
   * This will be used as our temp location for the page by page copy
   */
  for (i = MEM_INVALID_PAGES; i < USER_STACK_LIMIT/PAGESIZE; i++) {
    if (parent_page_table[i].valid == 0) {
      first_invalid_region_0_page = i;
      TracePrintf(3, "context_switch: got first invalid page from region 0: %d\n", first_invalid_region_0_page);
      break;
    }
  }

  TracePrintf(3, "context_switch: beginning region 0 copy.\n");

  //if we don't have enough physical memory to make the copy, return with parent saved context
  TracePrintf(3, "context_switch: num_user_pages: %d , num_free_physical_pages: %d\n", num_user_pages, num_free_physical_pages());
  if(num_user_pages - MEM_INVALID_PAGES > num_free_physical_pages()){
    parent_pcb->out_of_memory = 1;

    return &parent_pcb->saved_context;
  }

  // copy the region 0 of parent to child
  if(first_invalid_region_0_page != -1){
    TracePrintf(3, "context_switch: using the user stack for temp virtual page\n");

    for (i = MEM_INVALID_PAGES; i < num_user_pages; i++) {
      if (parent_page_table[i].valid == 1) {
        TracePrintf(3, "context_switch: copying page: %d\n", i);

        unsigned int child_physical_page_number = acquire_free_physical_page();
        
        TracePrintf(3, "context_switch: acquired free phys page.\n");

        // temporarily map that page to a physical page.
        parent_page_table[first_invalid_region_0_page].valid = 1;
        parent_page_table[first_invalid_region_0_page].kprot = PROT_READ | PROT_WRITE;
        parent_page_table[first_invalid_region_0_page].uprot = PROT_READ | PROT_EXEC;
        parent_page_table[first_invalid_region_0_page].pfn = child_physical_page_number;

        TracePrintf(3, "context_switch: done setting parent_page_table page table settings.\n");

        void *parent_virtual_addr = (void *)(long)((i * PAGESIZE) + VMEM_0_BASE);
        void *temp_virt_addr_for_region_0_page = (void *)(long)((first_invalid_region_0_page * PAGESIZE) + VMEM_0_BASE);

        WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) temp_virt_addr_for_region_0_page);

        TracePrintf(3, "context_switch: about to do memcopy, src: %p, dest: %p\n", parent_virtual_addr, temp_virt_addr_for_region_0_page);

        // copy region 0 page into our new page of memory.
        memcpy(
          temp_virt_addr_for_region_0_page, // dest
          parent_virtual_addr, // src
          PAGESIZE
        );

        TracePrintf(3, "context_switch: abiyt to change valid for parent_page_table entry we used.\n");

        // pretend that the temp memory never existed.
        parent_page_table[first_invalid_region_0_page].valid = 0;
        WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) temp_virt_addr_for_region_0_page);

        TracePrintf(3, "context_switch: done flushing TLB\n");

        // give the pfn from the temp memory to child's page table.
        child_page_table[i].valid = 1;
        child_page_table[i].pfn = child_physical_page_number;
      }
    }
  } else {
    //Use region 1 as our temp to copy the page tables
    int first_invalid_region_1_page = -1;

    for (i = 0; i < PAGE_TABLE_LEN; i++) {
      if (kernel_page_table[i].valid == 0) {
        first_invalid_region_1_page = i;

        parent_pcb->out_of_memory = 0;

        break;
      }
    }

    if (first_invalid_region_1_page == -1) {
      parent_pcb->out_of_memory = 1;

      // if we can't copy over the region_0, then we HAVE to use the parent's kernel stack.
      return &parent_pcb->saved_context;
    } else {
      // use the page to copy over the entire region_1.
      for (i = MEM_INVALID_PAGES; i < num_user_pages; i++) {
        if (parent_page_table[i].valid == 1) {
          unsigned int child_physical_page_number = acquire_free_physical_page();

          // temporarily map that page to a physical page.
          kernel_page_table[first_invalid_region_1_page].valid = 1;
          kernel_page_table[first_invalid_region_1_page].pfn = child_physical_page_number;

          void *parent_virtual_addr = (void *)(long)((i * PAGESIZE) + VMEM_0_BASE);
          void *temp_virt_addr_for_region_1_page = (void *)(long)((first_invalid_region_1_page * PAGESIZE) + VMEM_1_BASE);

          WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) temp_virt_addr_for_region_1_page);

          // copy region 0 page into our new page of memory.
          memcpy(
            temp_virt_addr_for_region_1_page, // dest
            parent_virtual_addr, // src
            PAGESIZE
          );

          // pretend that the temp memory never existed.
          parent_page_table[first_invalid_region_1_page].valid = 0;
          WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) temp_virt_addr_for_region_1_page);

          // give the pfn from the temp memory to child's page table.
          child_page_table[i].valid = 1;
          child_page_table[i].pfn = child_physical_page_number;
        }
      }
    }
  }

  TracePrintf(3, "%p\n", child_page_table);

  for (i = 0; i < VMEM_0_LIMIT/PAGESIZE; i++) {
    TracePrintf(3, "context_switch: %d, %d \n", i, child_page_table[i].valid);
  }

  WriteRegister(REG_PTR0, (RCS421RegVal)child_page_table);
  TracePrintf(3, "Breaking on flush?\n");
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

  TracePrintf(1, "context_switch: child_process_region_0_initialization() completed.\n");

  return &child_pcb->saved_context;  
}