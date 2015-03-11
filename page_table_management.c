#include "page_table_management.h"

struct pte *kernel_page_table;
struct pte *user_page_table;

//Kernel Page Table initialzation
void
init_kernel_page_table(){
  int i;
  kernel_page_table = malloc(PAGE_TABLE_SIZE);

  int end_of_text = ((long)&_etext - (long)VMEM_1_BASE) / PAGESIZE;
  int end_of_heap = ((long)kernel_brk - (long)VMEM_1_BASE) / PAGESIZE;

  for(i = 0; i < PAGE_TABLE_LEN; i++){
    if(i < end_of_text){
      kernel_page_table[i].valid = 1;
      kernel_page_table[i].kprot = 5; // 101
    } else if(i <= end_of_heap) {
      kernel_page_table[i].valid = 1;
      kernel_page_table[i].kprot = 3; // 110
    } else {
      kernel_page_table[i].valid = 0;
      kernel_page_table[i].kprot = 3; // 110
    }
    kernel_page_table[i].uprot = 0; // 000
    kernel_page_table[i].pfn = i + (long)VMEM_1_BASE/PAGESIZE;
  }
  TracePrintf(2, "Kernel page table initialized.\n");
}

void
init_user_page_table(){
  int i;
  user_page_table = malloc(PAGE_TABLE_SIZE);

  for(i = 0; i < PAGE_TABLE_LEN; i++) {
    if (i >= KERNEL_STACK_BASE / PAGESIZE) {
      user_page_table[i].valid = 1;
      user_page_table[i].kprot = 3;
      user_page_table[i].uprot = 0;
    } else {
      user_page_table[i].valid = 0;
      user_page_table[i].kprot = 0; // 000
      user_page_table[i].uprot = 6; // 110
    }
    user_page_table[i].pfn = i;
  }
  TracePrintf(2, "User page table initialized.\n");
}