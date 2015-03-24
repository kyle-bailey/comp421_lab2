#include "memory_management.h"
#include "page_table_management.h"

struct pte *kernel_page_table;

struct page_table_record *first_page_table_record;

struct page_table_record *
get_first_page_table_record() {
  return first_page_table_record;
}

// Note: This can only be called after virtual memory is enabled.
void
init_first_page_table_record() {
  struct page_table_record *page_table_record = malloc(sizeof(struct page_table_record));

  void *page_base = (void *)DOWN_TO_PAGE(VMEM_1_LIMIT - 1);

  page_table_record->page_base = page_base;
  page_table_record->is_top_full = 0;
  page_table_record->is_bottom_full = 0;
  page_table_record->next = NULL;

  unsigned int pfn = acquire_free_physical_page();

  int vpn = (long)page_base/PAGESIZE;

  kernel_page_table[vpn].valid = 1;
  kernel_page_table[vpn].pfn = pfn;
}

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
  TracePrintf(2, "page_table_management: Kernel page table initialized.\n");
}

void
prep_user_page_table(struct pte *page_table){
  int i;

  TracePrintf(2, "page_table_management: Starting to prep user page table.\n");
  
  for(i = 0; i < PAGE_TABLE_LEN; i++) {
    if (i >= KERNEL_STACK_BASE / PAGESIZE) {
      page_table[i].valid = 1;
      page_table[i].kprot = PROT_READ | PROT_WRITE;
      page_table[i].uprot = PROT_NONE;
    } else {
      page_table[i].valid = 0;
      page_table[i].kprot = PROT_NONE;
      page_table[i].uprot = PROT_READ | PROT_WRITE | PROT_EXEC;
    }
    page_table[i].pfn = i;
  }
  TracePrintf(2, "page_table_management: User page table initialized.\n");
}

//assumes that virtual memory has been enabled
int
num_pages_in_use(struct pte *page_table){
  int i;
  int count = 0;
  for(i = 0; i < PAGE_TABLE_LEN - KERNEL_STACK_PAGES; i++){
    if(page_table[i].valid == 1){
      count++;
    }
  }
  return count;
}