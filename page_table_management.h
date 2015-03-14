#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include "memory_management.h"

extern struct pte *kernel_page_table;

void init_kernel_page_table();
void prep_user_page_table();
int num_pages_in_use_by_current_process();