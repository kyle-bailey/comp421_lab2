#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include <string.h>
#include "process_control_block.h"

SavedContext * context_switch_helper(SavedContext *ctxp, void *p1, void *p2);
SavedContext * context_switch_helper_with_kernel_stack_copy(SavedContext *ctxp, void *p1, void *p2);