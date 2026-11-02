/*
 * 03_bss_vs_data.c - prove that BSS costs no space in the executable.
 *
 * The same 1 MB array is built two ways:
 *
 *   gcc -Wall -Wextra 03_bss_vs_data.c -o in_bss
 *   gcc -Wall -Wextra -DIN_DATA 03_bss_vs_data.c -o in_data
 *
 *   ls -l in_bss in_data      in_data is ~1 MB bigger on disk
 *   size  in_bss in_data      the 1 MB shows up under "bss" vs "data"
 *
 * Why: DATA holds non-zero initial values, and they must be stored in
 * the file. Even ONE non-zero element forces the whole array into DATA,
 * zeros included. BSS is all zeros by definition, so the file only
 * records its size, and the kernel maps zero-filled pages at load time.
 *
 * Once loaded, both use the same 1 MB of memory. Actually the kernel
 * only hands out a physical page when it is first touched ("demand
 * paging"), so an untouched BSS array costs almost no RAM either.
 */
#include <stdio.h>

#define SIZE (1024 * 1024)

#ifdef IN_DATA
static char big[SIZE] = { 1 };	/* one non-zero byte -> whole array in DATA */
#define WHERE "DATA"
#else
static char big[SIZE];		/* all zero -> BSS */
#define WHERE "BSS"
#endif

int main(void)
{
	printf("1 MB array in %s at %p, big[0] = %d\n", WHERE, (void *)big,
	       big[0]);
	puts("compare: ls -l in_bss in_data  and  size in_bss in_data");
	return 0;
}
