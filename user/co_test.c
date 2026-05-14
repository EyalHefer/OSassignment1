#include "kernel/types.h"
#include "user/user.h"

void test_basic(void) {
  printf("=== Basic co_yield test ===\n");

  int pid1 = getpid();
  int pid2 = fork();

  if(pid2 == 0) {

    while(1){
      int value = co_yield(pid1, 1);
      if(value < 0) 
        break;
      printf("Child received: %d\n", value);
    }
    exit(0);

  } else {

    for(int i = 0; i < 5; i++){
      int value = co_yield(pid2, 2);
      printf("Parent received: %d\n", value);
    }

    kill(pid2);
    wait(0);
  }
}