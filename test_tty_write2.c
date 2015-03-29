#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

int
main() {
  printf("Test tty_write2 Process Initialized.\n");
  int return_val;

  char *buf = malloc(sizeof(char) * 4);
  buf[0] = 'f';
  buf[1] = 'u';
  buf[2] = 'c';
  buf[3] = 'k';

  return_val = TtyWrite(0, buf, 5);
  printf("Wrote to terminal 0 and got return value: %d\n", return_val);

  return_val = TtyWrite(1, buf, 5);
  printf("Wrote to terminal 1 and got return value: %d\n", return_val);

  return_val = TtyWrite(2, buf, 5);
  printf("Wrote to terminal 2 and got return value: %d\n", return_val);

  return_val = TtyWrite(3, buf, 5);
  printf("Wrote to terminal 3 and got return value: %d\n", return_val);

  // return_val = TtyWrite(NUM_TERMINALS + 1, buf, 5);
  // printf("Wrote to terminal NUM_TERMINALS + 1 and got return value: %d\n", return_val);

  return 0;
}