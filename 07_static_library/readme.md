# Static Library Example

A static library (`.a` file on Linux) is just an archive of compiled `.o`
object files. It gets copied directly into your executable at link time,
so the final binary has no runtime dependency on the library file.

## Files
- `mymath.h`   - public function declarations
- `mymath.c`   - library implementation (add, sub, mul, div_int)
- `main.c`     - program that links against the static library

## Build steps

1. Compile the library source into an object file:
   ```
   gcc -c mymath.c -o mymath.o
   ```

2. Archive the object file into a static library:
   ```
   ar rcs libmymath.a mymath.o
   ```
   - `r` - insert/replace files in the archive
   - `c` - create the archive if it doesn't exist
   - `s` - write an index (symbol table) into the archive

3. Compile `main.c` and link it against the static library:
   ```
   gcc main.c -L. -lmymath -o main
   ```
   - `-L.` tells gcc to look for libraries in the current directory
   - `-lmymath` links `libmymath.a` (drop the `lib` prefix and `.a` suffix)

4. Run it:
   ```
   ./main
   ```

## One-liner
```
gcc -c mymath.c -o mymath.o && ar rcs libmymath.a mymath.o && gcc main.c -L. -lmymath -o main && ./main
```
