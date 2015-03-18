#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <comp421/hardware.h>
#include <comp421/loadinfo.h>

int LoadProgram(char *name, char **args, ExceptionStackFrame *frame, struct pte *page_table_to_load);