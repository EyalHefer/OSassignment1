#include "kernel/types.h"
#include "user/user.h"

static void
print_status(const char *label, int size)
{
  printf("%s: %d bytes\n", label, size);
}

int
main(int argc, char *argv[])
{
  int before, after_alloc, after_free;
  char *buf;

  before = memsize();
  print_status("before malloc", before);

  buf = malloc(20 * 1024);
  if(buf == 0){
    printf("memsize_test: FAIL (malloc returned null)\n");
    exit(1);
  }

  after_alloc = memsize();
  print_status("after malloc", after_alloc);

  free(buf);

  after_free = memsize();
  print_status("after free", after_free);

  if(after_alloc <= before){
    printf("memsize_test: FAIL (memory did not increase)\n");
    exit(1);
  }

  // In xv6 malloc, freeing returns block to user allocator, not to kernel;
  // process size usually stays the same after free().
  if(after_free != after_alloc){
    printf("memsize_test: FAIL (unexpected shrink after free)\n");
    exit(1);
  }

  printf("memsize_test: PASS\n");
  exit(0);
}
