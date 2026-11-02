/*
 * 01_memory_layout.c - where every kind of variable lives in memory.
 *
 * When Linux runs a program it gives the process a virtual address space
 * split into SECTIONS (also called segments). Low addresses at the bottom:
 *
 *   high address  +---------------------------+
 *                 | command-line args, env    |
 *                 +---------------------------+
 *                 | STACK                     |  local variables, function
 *                 |   grows DOWN  |           |  arguments, return addresses
 *                 |               v           |
 *                 |                           |
 *                 |   (free space, mmap,      |  shared libraries (libc.so),
 *                 |    shared libraries)      |  big malloc()s
 *                 |                           |
 *                 |               ^           |
 *                 |   grows UP    |           |
 *                 | HEAP                      |  malloc / calloc / realloc
 *                 +---------------------------+  <- &end  (program break)
 *                 | BSS                       |  globals & statics that are
 *                 |                           |  UNINITIALISED or = 0
 *                 +---------------------------+  <- &edata
 *                 | DATA                      |  globals & statics with a
 *                 |                           |  NON-ZERO initial value
 *                 +---------------------------+
 *                 | RODATA (read-only data)   |  string literals, const globals
 *                 +---------------------------+  <- &etext
 *                 | TEXT (code)               |  machine instructions:
 *   low address   +---------------------------+  main(), your functions
 *
 * STATIC vs DYNAMIC
 *   - TEXT, RODATA, DATA and BSS have a size fixed at COMPILE time.
 *     They exist from program start to program end ("static storage").
 *   - STACK and HEAP grow and shrink while the program runs ("dynamic").
 *     Stack memory is freed automatically when a function returns.
 *     Heap memory lives until YOU call free().
 *
 * WHY BSS EXISTS
 * A DATA variable's initial value must be stored IN the executable file,
 * so the loader can copy it into memory. BSS variables all start at 0, so
 * the file only records "I need N bytes"; the kernel hands out zero-filled
 * pages at load time. A 1 MB zero array costs 1 MB of file in DATA, but
 * almost nothing in BSS (see 03_bss_vs_data.c).
 * ("BSS" is a historical assembler name: "Block Started by Symbol".)
 *
 * Build:  gcc -Wall -Wextra -no-pie 01_memory_layout.c -o layout
 *         -no-pie keeps addresses small and readable; without it the
 *         sections keep their order but sit at random high addresses (ASLR).
 *
 * Inspect the binary:
 *         size layout                 text / data / bss sizes
 *         nm -n layout | grep g_      which section each global is in
 *         objdump -h layout           all sections with addresses
 */
#define _DEFAULT_SOURCE		/* for sbrk() */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Symbols the LINKER defines for us (see "man 3 end"). They aren't real
 * variables; only their ADDRESSES mean something:
 *   &etext  first address after the TEXT section
 *   &edata  first address after the DATA section
 *   &end    first address after the BSS section (heap starts near here)
 */
extern char etext, edata, end;

/* ---------- DATA: initialised to a NON-zero value ---------- */
int g_data = 42;			/* global, stored in the file */
static int g_static_data = 7;		/* static = visible in this file only;
					   still DATA, same lifetime */

/* ---------- BSS: no initialiser, or initialised to zero ---------- */
int g_bss;				/* C guarantees this starts at 0 */
int g_bss_zero = 0;			/* = 0 also goes to BSS */
static char g_bss_buf[4096];		/* 4 KB, but adds ~0 bytes to the file */

/* ---------- RODATA: read-only ---------- */
const int g_const = 100;		/* const global: writing it crashes */
const char *g_str = "hello";		/* the POINTER g_str is in DATA,
					   the characters "hello" in RODATA */

/* ---------- TEXT: code ---------- */
static void some_function(void) { }

/* Print one line: name, address, and which section it is in. */
static void show(const char *what, const void *addr, const char *section)
{
	printf("  %-34s %18p   %s\n", what, addr, section);
}

int main(void)
{
	/*
	 * A static LOCAL: visible only inside main, but stored with the
	 * globals (DATA here, because it's non-zero), not on the stack.
	 * It's created once at program start and keeps its value between
	 * calls. See 02_static_vs_dynamic.c.
	 */
	static int s_local_data = 5;
	static int s_local_bss;		/* zero -> BSS */

	/* ---------- STACK: ordinary local variables ---------- */
	int local = 1;
	char local_arr[16] = "on the stack";

	/* ---------- HEAP: allocated at run time ---------- */
	int *heap = malloc(sizeof(*heap));
	char *heap_str = malloc(32);

	if (!heap || !heap_str) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	*heap = 3;
	strcpy(heap_str, "on the heap");

	puts("Addresses, from LOW to HIGH section:\n");

	/*
	 * Casting a function pointer to void * is not strictly ISO C, but
	 * POSIX requires it to work (dlsym() depends on it).
	 */
	puts("TEXT (code, read-only + executable)");
	show("main()", (void *)main, "text");
	show("some_function()", (void *)some_function, "text");
	show("&etext (end of text)", &etext, "-- boundary --");

	puts("RODATA (read-only data)");
	show("\"hello\" string literal", g_str, "rodata");
	show("&g_const", &g_const, "rodata");

	puts("DATA (initialised globals/statics)");
	show("&g_data            = 42", &g_data, "data");
	show("&g_static_data     = 7", &g_static_data, "data");
	show("&s_local_data      = 5 (static)", &s_local_data, "data");
	show("&g_str (the pointer itself)", &g_str, "data");
	show("&edata (end of data)", &edata, "-- boundary --");

	puts("BSS (zero / uninitialised globals/statics)");
	show("&g_bss", &g_bss, "bss");
	show("&g_bss_zero        = 0", &g_bss_zero, "bss");
	show("&s_local_bss       (static)", &s_local_bss, "bss");
	show("g_bss_buf[4096]", g_bss_buf, "bss");
	show("&end (end of bss)", &end, "-- boundary --");

	puts("HEAP (malloc)");
	show("heap   (malloc 4 bytes)", heap, "heap");
	show("heap_str (malloc 32 bytes)", heap_str, "heap");
	/*
	 * sbrk(0) returns the current "program break": the top of the heap.
	 * malloc moves it up with brk() when it needs more small memory.
	 */
	show("sbrk(0) = top of heap", sbrk(0), "-- boundary --");

	puts("STACK (locals, grows down)");
	show("&local", &local, "stack");
	show("local_arr", local_arr, "stack");

	/* BSS really does start at zero: */
	printf("\ng_bss = %d, s_local_bss = %d, g_bss_buf[100] = %d"
	       "  (all zero, no initialiser needed)\n",
	       g_bss, s_local_bss, g_bss_buf[100]);

	/*
	 * The kernel's own view of this process's memory map. Each line is
	 * one region: address range, permissions (r=read w=write x=execute
	 * p=private), and what it is. Notice:
	 *   r--p  ./layout   = ELF headers, RODATA   (read only)
	 *   r-xp  ./layout   = TEXT       (executable, not writable)
	 *   rw-p  ./layout   = DATA+BSS   (writable, not executable)
	 *   [heap], [stack]  = dynamic regions
	 */
	puts("\n/proc/self/maps (the kernel's view):");
	fflush(stdout);
	/* /proc/self/exe is a link to our own executable: get its path. */
	char exe[512] = "";
	ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);

	if (len > 0)
		exe[len] = '\0';

	FILE *f = fopen("/proc/self/maps", "r");
	if (f) {
		char line[1024];

		while (fgets(line, sizeof(line), f))
			if ((len > 0 && strstr(line, exe)) ||
			    strstr(line, "[heap]") ||
			    strstr(line, "[stack]"))
				printf("  %s", line);
		fclose(f);
	}

	free(heap);
	free(heap_str);
	return EXIT_SUCCESS;
}
