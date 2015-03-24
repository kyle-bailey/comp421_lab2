#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

int
main() {
  printf("Test GetPid Process Initialized.\n");
  printf("%d\n", GetPid());

  return 0;
}