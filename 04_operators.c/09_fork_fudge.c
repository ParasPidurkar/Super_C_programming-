#include <stdio.h>
#include <unistd.h>

int main(void) {

  fork() && (fork() || fork());

  printf("forked!\n");
  return 0;
}