#include <comp421/hardware.h>
#include <comp421/yalnix.h>

extern int *is_page_occupied;
extern void *kernel_brk;

void occupy_kernel_pages_up_to(void *end);
void occupy_pages_in_range(void *begin, void *end);