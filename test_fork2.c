#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

int
main() {
  printf("Test fork2 Process Initialized.\n");

  int should_be_copied = 5;

  char *big_string = malloc(sizeof(char) * 4096);
  big_string[0] = 'h';
  big_string[1] = 'i';
  big_string[2] = '\0';

  int child_pid = Fork();

  printf("My Pid is: %d\n", GetPid());
  printf("Fork() return value is: %d\n", child_pid);

  printf("Stack variable that should have been copied: %d\n", should_be_copied);
  printf("Heap variable spanning multiple pages: %s\n", big_string);
  return 0;
}