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
      kernel_page_table[i].kprot = PROT_READ | PROT_EXEC;
    } else if(i <= end_of_heap) {
      kernel_page_table[i].valid = 1;
      kernel_page_table[i].kprot = PROT_READ | PROT_WRITE;
    } else {
      kernel_page_table[i].valid = 0;
      kernel_page_table[i].kprot = PROT_READ | PROT_WRITE;
    }
    kernel_page_table[i].uprot = PROT_NONE;
    kernel_page_table[i].pfn = i + (long)VMEM_1_BASE/PAGESIZE;
  }
  TracePrintf(2, "Kernel page table initialized.\n");
}

void
prep_user_page_table(struct pte *page_table){
  int i;
  page_table = *page_table_ptr;

  for(i = 0; i < PAGE_TABLE_LEN; i++) {
    if (i >= KERNEL_STACK_BASE / PAGESIZE) {
      page_table[i].valid = 1;
      page_table[i].kprot = PROT_READ | PROT_WRITE;
      page_table[i].uprot = PROT_NONE;
    } else {
      page_table[i].valid = 0;
      page_table[i].kprot = PROT_NONE;
      page_table[i].uprot = PROT_READ | PROT_EXEC;
    }
    page_table[i].pfn = i;
  }
  TracePrintf(2, "User page table initialized.\n");
}

//assumes that virtual memory has been enabled
int
num_pages_in_use_by_current_process(){
  int i;
  int count = 0;
  for(i = 0; i < PAGE_TABLE_LEN - KERNEL_STACK_PAGES; i++){
    if(user_page_table[i].valid == 1){
      count++;
    }
  }
  return count;
}