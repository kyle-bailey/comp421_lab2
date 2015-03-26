#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

int
main() {
  printf("Test fork Process Initialized.\n");

  int child_pid = Fork();

  printf("My Pid is: %d\n", GetPid());
  printf("child_pid is: %d\n", child_pid);
  return 0;
}