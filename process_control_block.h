#include <comp421/hardware.h>
#include <comp421/yalnix.h>

struct process_control_block
{
  int pid;
  struct pte *page_table[((long)VMEM_0_LIMIT - (long)VMEM_0_BASE)/PAGESIZE];
  void *stack_ptr;
  SavedContext saved_context;
};