#include <comp421/hardware.h>
#include <comp421/yalnix.h>

extern struct pte *kernel_page_table;
extern struct pte *user_page_table;

void init_kernel_page_table();
void init_user_page_table();