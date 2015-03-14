#include <comp421/hardware.h>
#include <comp421/yalnix.h>

struct process_control_block
{
  int pid;
  struct pte *page_table;
  SavedContext *saved_context;
};