#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

int
main() {
  printf("brk_test Process Initialized.\n");
  printf("PID: %d\n", GetPid());

  printf("Size of a char: %lu\n", sizeof(char));

  char *bigstring = malloc(sizeof(char) * 4096);

  printf("bigstring ptr: %p\n", bigstring);

  bigstring[0] = 'h';
  bigstring[1] = 'i';
  bigstring[2] = '\0';

  printf("%s\n", bigstring);

  free(bigstring);

  printf("we freed willy: %p\n", bigstring);

  Brk((void *)0x1e008);

  return 0;
}