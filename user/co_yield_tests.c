/*
 * Task 3 userspace tests for co_yield.
 * Run inside xv6: co_yield_tests
 */

#include "kernel/types.h"
#include "user/user.h"

static int g_failures;

static void
fail(const char *msg)
{
  printf("FAIL: %s\n", msg);
  g_failures++;
}

static void
pass(const char *msg)
{
  printf("PASS: %s\n", msg);
}

static void
test_self_yield(void)
{
  printf("\n=== test_self_yield ===\n");
  if(co_yield(getpid(), 42) != -1)
    fail("yield to own pid must return -1");
  else
    pass("self-yield returns -1");
}

static void
test_invalid_pid(void)
{
  printf("\n=== test_invalid_pid ===\n");

  if(co_yield(0, 1) != -1)
    fail("pid 0 must return -1");
  else
    pass("pid 0");

  if(co_yield(-3, 1) != -1)
    fail("negative pid must return -1");
  else
    pass("negative pid");

  /* Unlikely to exist on a fresh boot with few processes */
  if(co_yield(314159, 1) != -1)
    fail("non-existent pid must return -1");
  else
    pass("non-existent pid");
}

static void
test_killed_target(void)
{
  printf("\n=== test_killed_target ===\n");

  int pid = fork();
  if(pid < 0){
    fail("fork failed");
    return;
  }

  if(pid == 0){
    sleep(10000);
    exit(0);
  }

  kill(pid);

  if(co_yield(pid, 99) != -1)
    fail("yield to killed process must return -1");
  else
    pass("killed target");

  wait(0);
}

/*
 * Same pattern as the assignment example: child loops until killed,
 * parent runs a fixed number of successful exchanges.
 */
static void
test_pingpong_assignment_pattern(void)
{
  const int rounds = 50;
  printf("\n=== test_pingpong_assignment_pattern (%d rounds) ===\n",
         rounds);

  int parent_pid = getpid();
  int child_pid = fork();

  if(child_pid < 0){
    fail("fork failed");
    return;
  }

  if(child_pid == 0){
    for(;;){
      int v = co_yield(parent_pid, 1);
      if(v < 0)
        exit(0);
      if(v != 2){
        printf("FAIL: child expected value 2, got %d\n", v);
        exit(1);
      }
    }
  }

  for(int i = 0; i < rounds; i++){
    int v = co_yield(child_pid, 2);
    if(v != 1){
      printf("FAIL: parent iter %d expected 1, got %d\n", i, v);
      kill(child_pid);
      wait(0);
      exit(1);
    }
  }

  kill(child_pid);
  wait(0);
  pass("ping-pong values match assignment pattern");
}

int
main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  g_failures = 0;

  printf("co_yield_tests - Task 3\n");

  test_self_yield();
  test_invalid_pid();
  test_killed_target();
  test_pingpong_assignment_pattern();

  if(g_failures){
    printf("\nco_yield_tests: finished with %d failed check(s)\n", g_failures);
    exit(1);
  }

  printf("\nco_yield_tests: all checks passed\n");
  exit(0);
}
