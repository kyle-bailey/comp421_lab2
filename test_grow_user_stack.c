#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

int
main() {
  printf("Test grow user stack Process Initialized.\n");
  printf("Initializing local variable big array\n");
  int bigarray[4096] = {0};
  bigarray[0] = 1;
  printf("bigarray initialized.\n");

  return 0;
}