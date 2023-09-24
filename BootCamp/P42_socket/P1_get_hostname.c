#include <stdio.h>
#include <sys/unistd.h>
int main(void)
{
char hostname[128];
gethostname(hostname, sizeof(hostname));
printf("My local hostname: %s\n", hostname);
return 0;
}