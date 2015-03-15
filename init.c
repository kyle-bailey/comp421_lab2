#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>

int
main() {
  printf("Init Process Initialized.\n");
  printf("%d\n", GetPid());

  return 0;
}