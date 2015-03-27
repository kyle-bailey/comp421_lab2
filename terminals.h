#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>

struct charbuffer *charbuffers;

struct charbuffer {
  char buffer[TERMINAL_MAX_LINE];
  int read;
  int write;
  int count;
};

void init_charbuffers();

int write_to_buffer_raw(int terminal, char *buf, int len);

int write_to_buffer(int terminal, char *buf, int len);

int read_from_buffer(int terminal, char *buf, int len);

int new_line_in_buffer(int terminal);