#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

int
main() {
  printf("Test wait Process Initialized.\n");

  int *status_value = malloc(sizeof(int));

  Fork();
  printf("%d is about to wait\n", GetPid());
  int wait_return_value = Wait(status_value);
  Delay(5);
  printf("Delay finished for %d, Wait returned %d, status value is %d\n", GetPid(), wait_return_value, *status_value);

  return 1;
}