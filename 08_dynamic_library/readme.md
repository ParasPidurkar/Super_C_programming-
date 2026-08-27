# Dynamic (Shared) Library Example

A dynamic library (`.so` file on Linux) is NOT copied into the executable.
The executable only stores a reference to it, and the library is loaded
into memory at **runtime** by the dynamic linker (`ld.so`). Multiple
programs can share one copy of the library in memory, and you can update
the `.so` file without recompiling the programs that use it.

## Files
- `mymath.h`   - public function declarations
- `mymath.c`   - library implementation (add, sub, mul, div_int)
- `main.c`     - program that links against the shared library

## Build steps

1. Compile the library source into position-independent object code:
   ```
   gcc -c -fPIC mymath.c -o mymath.o
   ```
   - `-fPIC` generates Position Independent Code, required for shared
     libraries because they can be loaded at any memory address.

2. Create the shared library from the object file:
   ```
   gcc -shared -o libmymath.so mymath.o
   ```

3. Compile `main.c` and link it against the shared library:
   ```
   gcc main.c -L. -lmymath -o main
   ```
   - `-L.` tells gcc to look for libraries in the current directory
   - `-lmymath` links `libmymath.so` (drop the `lib` prefix and `.so` suffix)

4. Tell the dynamic linker where to find `libmymath.so` at runtime, then run:
   ```
   LD_LIBRARY_PATH=. ./main
   ```
   Without this, `./main` will fail with an error like:
   `error while loading shared libraries: libmymath.so: cannot open shared object file`
   (This is only needed because the library lives outside the standard
   search paths like `/usr/lib`. Installing it there, or adding its
   directory to `/etc/ld.so.conf`, would remove the need for this.)

## One-liner
```
gcc -c -fPIC mymath.c -o mymath.o && gcc -shared -o libmymath.so mymath.o && gcc main.c -L. -lmymath -o main && LD_LIBRARY_PATH=. ./main
```

## Static vs Dynamic
| | Static (`.a`) | Dynamic (`.so`) |
|---|---|---|
| Linked | At compile time | At runtime |
| Copied into executable | Yes | No (loaded separately) |
| Executable size | Larger | Smaller |
| Update library without rebuilding | No | Yes |
| See also | [`07_static_library`](../07_static_library) | |
