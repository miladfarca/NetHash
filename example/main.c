#include "../src/nethash.h"
#include "../src/flag.h"
#include <stdio.h>

int main(int argc, char **argv) {

  const char* string = "Hello, NetHash!";
  unsigned char nethash_buffer[NETHASH_SIZE];

  // Setup flags.
  flag_parse_args(argc, argv);

  if (nethash_compute(string, nethash_buffer) != 0){
    fprintf(stderr, "Error occurred, run with `-v` to see the details.\n");
    return -1;
  }

  for (int i=0; i< NETHASH_SIZE; i++){
    printf("%02x", nethash_buffer[i]);
  }
  printf("\n");

  return 0;
}

