#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include <string.h>

extern int *is_page_occupied;
extern void *kernel_brk;
extern int virt_mem_initialized;

void occupy_kernel_pages_up_to(void *end);
void occupy_pages_in_range(void *begin, void *end);
void init_is_page_occupied(unsigned int pmem_size);