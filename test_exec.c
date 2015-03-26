#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

int
main() {
  printf("Test exec Process Initialized.\n");

  char *loadargs[1];
  loadargs[0] = NULL;

  Exec("test_getpid", loadargs);

  return 0;
}