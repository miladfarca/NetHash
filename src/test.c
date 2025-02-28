#include "nethash.h"
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "test.h"

// https://stackoverflow.com/a/26839647
static unsigned char hex_to_int(unsigned char ch){
  if (ch >= '0' && ch <= '9')
    return ch - '0';
  if (ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  if (ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;
  return -1;
}

#define RUN(test_name, nput, expectation)                                                 \
  unsigned char nethash_buffer[NETHASH_SIZE];                                             \
  nethash_compute(input, nethash_buffer);                                                 \
  for (int i = 0, j = 0; i < NETHASH_SIZE; i += 2, j++){                                  \
    unsigned char e = (hex_to_int(expectation[i]) << 4) | hex_to_int(expectation[i + 1]); \
    t_assert(test_name " failed", (unsigned int) nethash_buffer[j] == e);                 \
  }

int tests_run = 0;

static char *test_1(){
  const char* input = "Hello, NetHash!";
  const char* expectation = "d5ec5f72f3178db2ca4cb11b36469230686838288a87edcd6322b6178494948747e1a02d65272fcf";
  RUN("test_1", input, expectation);

  return 0;
}

static char *test_2(){
  const char* input = "";
  const char* expectation = "970f6ecfb08d82bfe3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
  RUN("test_2", input, expectation);

  return 0;
}

static char *test_3(){
  const char* input = " ";
  const char* expectation = "1127f1098ba3636a36a9e7f1c95b82ffb99743e0c5c4ce95d83c9a430aac59f84ef3cbfab6145068";
  RUN("test_3", input, expectation);

  return 0;
}

static char *test_4(){
  const char* input = "~!@#$%^&*()_+{}\":?><";
  const char* expectation = "3075037bd3212428609c88bc563168478b6d236fbdb12659bbdb58991ae96f521ec494aa7b203a43";
  RUN("test_4", input, expectation);

  return 0;
}

static char *test_5(){
  const char* input = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor.";
  const char* expectation = "272d024e9216e3ce61cce8c2ff996f2e87d6c797b35a5d163600b2385f121a2c6c6eb44dacd928fc";
  RUN("test_5", input, expectation);

  return 0;
}

static char *all_tests(){
  t_run_test(test_1);
  t_run_test(test_2);
  t_run_test(test_3);
  t_run_test(test_4);
  t_run_test(test_5);
  return 0;
}

int main(){
  struct timeval start, end;
  gettimeofday(&start, NULL);
  char *result = all_tests();
  gettimeofday(&end, NULL);

  double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

  if (result != 0){
    printf("%s\n", result);
  }
  else {
    printf("ALL TESTS PASSED\n");
  }
  printf("Tests run: %d\n", tests_run);
  printf("Time spent: %.3f seconds.\n", time_spent);

  return result != 0;
}
