#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

int
main() {
  printf("Test tty_write Process Initialized.\n");

  char *buf = malloc(sizeof(char) * 4);
  buf[0] = 'f';
  buf[1] = 'u';
  buf[2] = 'c';
  buf[3] = 'k';

  TtyWrite(0, buf, 5);

  printf("Wrote to terminal 0\n");

  return 0;
}