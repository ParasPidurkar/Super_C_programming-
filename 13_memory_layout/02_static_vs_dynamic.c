/*
 * 02_static_vs_dynamic.c - LIFETIME of memory: static, stack, heap.
 *
 * 01_memory_layout.c showed WHERE things live. This file shows HOW LONG
 * they live, which is what causes most memory bugs in C.
 *
 *   Storage    Where        Created           Destroyed          Size
 *   ---------- ------------ ----------------- ------------------ ----------
 *   static     data / bss   program start     program end        compile time
 *   automatic  stack        function entry    function return    compile time*
 *   dynamic    heap         malloc()          free()             run time
 *
 *   (* except variable-length arrays, which are best avoided)
 *
 * The keyword "static" means two different things in C:
 *   - on a LOCAL variable:   static STORAGE: lives for the whole program
 *   - on a GLOBAL/function:  internal LINKAGE: visible in this file only
 *
 * Build:  gcc -Wall -Wextra 02_static_vs_dynamic.c -o lifetime
 */
#define _DEFAULT_SOURCE		/* for sbrk() */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ---------- 1. static local vs normal local ---------- */
static void count_calls(void)
{
	/*
	 * auto_count is on the STACK: created fresh (= 0) on every call
	 * and gone when the function returns.
	 *
	 * static_count is in BSS: initialised ONCE, before main runs, and
	 * it keeps its value between calls.
	 */
	int auto_count = 0;
	static int static_count;

	auto_count++;
	static_count++;
	printf("  auto_count = %d, static_count = %d\n", auto_count,
	       static_count);
}

/* ---------- 2. returning memory from a function ---------- */

/*
 * WRONG (left commented out; gcc warns about it):
 *
 *	char *bad_name(void)
 *	{
 *		char buf[16] = "dangling";
 *		return buf;   // buf is on the stack and dies HERE
 *	}
 *
 * The caller gets the address of stack memory that the next function
 * call will overwrite: a "dangling pointer". It may even seem to work,
 * which makes the bug hard to find.
 */

/* RIGHT #1: static buffer. Valid forever, but SHARED by every call. */
static char *static_name(const char *s)
{
	static char buf[32];

	snprintf(buf, sizeof(buf), "static:%s", s);
	return buf;	/* fine: buf is not on the stack */
}

/* RIGHT #2: heap buffer. Each call gets its own; the caller must free. */
static char *heap_name(const char *s)
{
	char *buf = malloc(32);

	if (buf)
		snprintf(buf, 32, "heap:%s", s);
	return buf;
}

/* ---------- 3. the stack grows down ---------- */
static void recurse(int depth, const char *prev)
{
	char here;	/* one byte on this call's stack frame */

	if (prev)
		printf("  depth %d: &here = %p  (%td bytes below the caller)\n",
		       depth, (void *)&here, prev - &here);
	else
		printf("  depth %d: &here = %p\n", depth, (void *)&here);
	if (depth < 3)
		recurse(depth + 1, &here);
}

int main(void)
{
	puts("1. static local keeps its value, normal local does not");
	for (int i = 0; i < 3; i++)
		count_calls();

	puts("\n2. returning memory from a function");
	char *a = static_name("first");
	char *b = static_name("second");
	/*
	 * Both pointers are the SAME buffer, so the second call overwrote
	 * the first result. That is why functions like strtok, asctime and
	 * inet_ntoa (which use a static buffer) are not thread-safe.
	 */
	printf("  static_name: a = \"%s\", b = \"%s\", a == b: %s\n", a, b,
	       a == b ? "yes (same buffer!)" : "no");

	char *c = heap_name("first");
	char *d = heap_name("second");
	if (!c || !d) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	printf("  heap_name:   c = \"%s\", d = \"%s\", c == d: %s\n", c, d,
	       c == d ? "yes" : "no (separate blocks)");
	free(c);	/* the caller owns heap memory: free it exactly once */
	free(d);

	puts("\n3. each nested call's frame sits LOWER on the stack");
	recurse(0, NULL);

	puts("\n4. heap: size chosen at run time, can grow with realloc");
	size_t n = 4;
	int *arr = malloc(n * sizeof(*arr));
	if (!arr) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	for (size_t i = 0; i < n; i++)
		arr[i] = (int)i * 10;
	printf("  arr at %p holds %zu ints\n", (void *)arr, n);

	/*
	 * realloc may MOVE the block: it allocates a bigger one, copies the
	 * old contents and frees the old block. So:
	 *   - always use the returned pointer, never the old one;
	 *   - assign to a temporary first, so a failure (NULL) doesn't lose
	 *     the original block, which would be a leak.
	 */
	int *tmp = realloc(arr, 1000 * sizeof(*arr));
	if (!tmp) {
		free(arr);
		perror("realloc");
		return EXIT_FAILURE;
	}
	arr = tmp;
	printf("  after realloc to 1000 ints: %p, arr[3] = %d (kept)\n",
	       (void *)arr, arr[3]);
	free(arr);
	arr = NULL;	/* good habit: a later free(NULL) is harmless,
			   while use of a stale pointer would crash loudly */

	puts("\n5. small vs big malloc: brk heap vs mmap");
	/*
	 * glibc's malloc gets memory from the kernel in two ways:
	 *   - small requests: carve from the [heap], growing it with brk();
	 *     the program break (sbrk(0)) moves up.
	 *   - big requests (>= 128 KB by default, M_MMAP_THRESHOLD): a
	 *     separate mmap() region, far from the heap, returned to the
	 *     kernel immediately on free().
	 */
	void *brk0 = sbrk(0);
	char *small = malloc(1024);
	char *big = malloc(1024 * 1024);
	if (!small || !big) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	printf("  program break:   %p\n", brk0);
	printf("  small (1 KB):    %p   near the break -> [heap]\n",
	       (void *)small);
	printf("  big   (1 MB):    %p   far away       -> mmap region\n",
	       (void *)big);
	free(small);
	free(big);

	/*
	 * Common heap bugs (try them under: gcc -fsanitize=address -g):
	 *   leak            malloc without free
	 *   double free     free(p); free(p);
	 *   use after free  free(p); p[0] = 1;
	 *   overflow        p = malloc(4); p[4] = 1;
	 */
	return EXIT_SUCCESS;
}
