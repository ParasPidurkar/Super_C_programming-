# 12 - C++ Templates

Templates let you write code once with a placeholder type (`T`). The compiler
then generates a concrete version for each type you use. All of this happens
at compile time, so it costs nothing at runtime.

| File | Topic |
|------|-------|
| `01_function_template.cpp` | basic template, type deduction, multiple params, non-type param (array size), explicit `<T>`, full specialization, template vs normal overload |
| `02_class_template.cpp` | `Stack<T, N>`, members defined outside the class, default args, `Pair<K,V>`, template member function, full and partial specialization, CTAD |

## Build & run
```sh
g++ -std=c++17 -Wall -Wextra 01_function_template.cpp -o func_tmpl && ./func_tmpl
g++ -std=c++17 -Wall -Wextra 02_class_template.cpp   -o class_tmpl && ./class_tmpl
```

## Key rules
- `template <typename T>` and `template <class T>` mean the same thing.
- The compiler only generates code for a template when it is used (instantiation).
- `Stack<int>` and `Stack<double>` are different, unrelated types.
- Keep template definitions in the header. If you split them into `.h` + `.cpp`, the linker reports "undefined reference" errors.
- Functions only allow **full** specialization. Classes allow both **full** and **partial** (e.g. `Box<T*>`) specialization.
- With `max_of("a", "b")` the generic template compares **pointer addresses**, so you need a specialization that uses `strcmp`.
