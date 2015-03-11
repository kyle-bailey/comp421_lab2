#include <stdio.h>
#include <comp421/hardware.h>

int
main() {
  printf("Idle Processing Initialized.\n");
  while(1) {
    Pause();
  }
}