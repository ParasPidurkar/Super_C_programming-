# 13 - Memory layout: text, rodata, data, bss, heap, stack

Every running C program gets its memory split into sections:

```
high  ┌──────────────────┐
      │ stack   ↓        │  locals, args, return addresses      (dynamic, automatic)
      │                  │
      │ mmap / libs      │  shared libraries, big malloc()s
      │                  │
      │ heap    ↑        │  malloc / calloc / realloc           (dynamic, manual free)
      ├──────────────────┤  &end
      │ bss              │  globals/statics = 0 or no init      (static)
      ├──────────────────┤  &edata
      │ data             │  globals/statics with non-zero init  (static)
      ├──────────────────┤
      │ rodata           │  string literals, const globals      (static, read-only)
      ├──────────────────┤  &etext
      │ text             │  machine code: main(), functions     (static, read-only + exec)
low   └──────────────────┘
```

| Declaration | Section | Lifetime |
|---|---|---|
| `int g = 42;` (global) | data | whole program |
| `int g;` or `int g = 0;` | bss | whole program |
| `static int s = 5;` inside a function | data | whole program |
| `static int s;` inside a function | bss | whole program |
| `const int c = 100;` (global) | rodata | whole program |
| `"hello"` string literal | rodata | whole program |
| `int x;` inside a function | stack | until the function returns |
| `malloc(n)` | heap (or mmap if ≥ 128 KB) | until `free()` |
| functions | text | whole program |

| File | Shows |
|---|---|
| `01_memory_layout.c` | the address of each kind of variable, the linker symbols `etext`/`edata`/`end`, and `/proc/self/maps` |
| `02_static_vs_dynamic.c` | static locals keeping their value, dangling stack pointers, static vs heap return buffers, the stack growing down, `realloc`, brk heap vs mmap |
| `03_bss_vs_data.c` | the same 1 MB array: 16 KB executable in bss, 1 MB executable in data |

## Build and run

```sh
gcc -Wall -Wextra -no-pie 01_memory_layout.c -o layout && ./layout
gcc -Wall -Wextra 02_static_vs_dynamic.c -o lifetime && ./lifetime

gcc -Wall -Wextra 03_bss_vs_data.c -o in_bss
gcc -Wall -Wextra -DIN_DATA 03_bss_vs_data.c -o in_data
ls -l in_bss in_data
size  in_bss in_data
```

`-no-pie` gives small fixed addresses (`0x401000`...), so the order of sections is easy to read. Without it, ASLR moves everything to random high addresses on each run, but the order stays the same.

## Inspecting a binary

```sh
size layout                 # text / data / bss sizes in bytes
nm -n layout | grep g_      # symbols sorted by address, with section letter
objdump -h layout           # every section: .text .rodata .data .bss ...
readelf -S layout           # same, more detail
cat /proc/<pid>/maps        # memory map of a running process
```

`nm` section letters (uppercase = global, lowercase = static/file-local):

| Letter | Section |
|---|---|
| `T` / `t` | text (code) |
| `R` / `r` | rodata |
| `D` / `d` | data |
| `B` / `b` | bss |
| `U` | undefined: comes from another file or library (e.g. `printf`) |

## Key points

- **`static` on a local** gives it static storage. It lives in data/bss, is set up once, and keeps its value between calls.
- **`static` on a global or function** limits it to the current file (internal linkage). Its storage doesn't change.
- **BSS costs no file space.** Only its size is recorded, and the kernel provides zeroed pages at load time.
- **Never return the address of a local variable.** The stack frame is gone after the return.
- **Writing to rodata crashes** with SIGSEGV: `char *p = "hi"; p[0] = 'H';`. Use `char p[] = "hi";` to get a writable copy on the stack.
- **Every `malloc` needs exactly one `free`.** Check heap bugs with `gcc -fsanitize=address -g`.
