#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

int
main() {
  printf("Test tty_read Process Initialized.\n");

  char *readbuffer = malloc(sizeof(char) * 10);

  TtyRead(0, readbuffer, 5);

  printf("%s\n", readbuffer);

  return 0;
}