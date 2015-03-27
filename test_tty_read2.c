#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

int
main() {
  printf("Test tty_read Process Initialized.\n");

  char *readbuffer = malloc(sizeof(char) * 10);

  int ret_val = TtyRead(0, readbuffer, 5);
  printf("Read from terminal 2: %s with ret_val: %d\n", readbuffer, ret_val);

  ret_val = TtyRead(1, readbuffer, 5);
  printf("Read from terminal 2: %s with ret_val: %d\n", readbuffer, ret_val);

  ret_val = TtyRead(2, readbuffer, 5);
  printf("Read from terminal 2: %s with ret_val: %d\n", readbuffer, ret_val);


  return 0;
}