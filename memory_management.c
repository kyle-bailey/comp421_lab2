#include "memory_management.h"

//int array that keeps track of what pages are free (0 means free, 1 means occupied)
int *is_physical_page_occupied = NULL;
void *kernel_brk = (void *)VMEM_1_BASE;
int num_physical_pages;

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
  if(is_physical_page_occupied != NULL){
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
    is_physical_page_occupied[i] = 1;
  }
}

void
init_is_physical_page_occupied(unsigned int pmem_size) {
  //note that this will mark physical pages as occupied starting from VMEM_1_BASE up to whatever
  //is_page_occupied needs.
  num_physical_pages = pmem_size/PAGESIZE;
  is_physical_page_occupied = malloc(num_physical_pages * sizeof(int));

  memset(is_physical_page_occupied, 0, sizeof(is_physical_page_occupied));
}

int
num_free_physical_pages() {
  int count = 0;
  int i;

  for(i = 0; i < VMEM_0_LIMIT/PAGESIZE; i++){
    if(is_physical_page_occupied[i] == 0){
      count++;
    }
  }

  return count;
}

void
free_physical_page(unsigned int pfn) {
  is_physical_page_occupied[pfn] = 0;
}

unsigned int
aquire_free_physical_page() {
  int i;
  for(i = 0; i < pmem_size/PAGESIZE; i++){
    if(is_physical_page_occupied[i] == 0){
      is_physical_page_occupied[i] = 1;
      return i;
    }
  }
  Halt();
}