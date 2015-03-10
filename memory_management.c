#include "memory_management.h"

//int array that keeps track of what pages are free (0 means free, 1 means occupied)
int *is_page_occupied;
void *kernel_brk = (void *)VMEM_1_BASE;

int virt_mem_initialized = 0;

int SetKernelBrk(void *addr) {
  if(virt_mem_initialized) {
    //more complicated stuff
  } else {
    // SetKernelBrk should never be freeing a page we have already allocated.
    if ((long)addr <= (long)kernel_brk - PAGESIZE) {
      return -1;
    }
    occupy_kernel_pages_up_to(addr);
  }

  return 0;
}

void
occupy_kernel_pages_up_to(void *end) {
  if(is_page_occupied != NULL){
    occupy_pages_in_range(kernel_brk, end);
  }
  kernel_brk = (void *)UP_TO_PAGE(end);
}

void
occupy_pages_in_range(void *begin, void *end) {
  int i;
  int boundary = (long)UP_TO_PAGE(end)/PAGESIZE;
  int start = (long)DOWN_TO_PAGE(begin)/PAGESIZE;

  for(i = start; i < boundary; i++){
    is_page_occupied[i] = 1;
  }
}

void
init_is_page_occupied(unsigned int pmem_size) {
  //note that this will mark physical pages as occupied starting from VMEM_1_BASE up to whatever
  //is_page_occupied needs.
  is_page_occupied = malloc(pmem_size/PAGESIZE * sizeof(int));

  memset(is_page_occupied, 0, sizeof(is_page_occupied));
}