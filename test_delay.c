#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

int
main() {
  printf("Test Delay Process Initialized.\n");

  Delay(5);

  printf("First delay finished.\n");

  Delay(5);

  printf("Second delay finished.\n");

  return 0;
}