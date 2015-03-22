#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>

int
main() {
  printf("Init Process Initialized.\n");
  printf("PID: %d\n", GetPid());

  printf("Doing Delay now...");

  Delay(5);

  return 0;
}