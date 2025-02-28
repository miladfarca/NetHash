#include "flag.h"
#include <string.h>

int flag__assert = 0;
int flag__verbose = 0;

void flag_parse_args(int argc, char **argv)
{
  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "-a") == 0){
      flag__assert = 1;
    }
    else if (strcmp(argv[i], "-v") == 0){
      flag__verbose = 1;
    }
  }
}
