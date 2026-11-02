/*
 * 01_function_template.cpp
 *
 * FUNCTION TEMPLATES
 * ------------------
 * A template is a "blueprint" from which the COMPILER generates real code.
 * You write the logic once with a placeholder type (T), and the compiler
 * creates a separate concrete function for every type you actually use.
 *
 *     template <typename T>      // "T is some type, decided later"
 *     T max_of(T a, T b) { ... }
 *
 * When you call max_of(3, 7), the compiler sees two ints, deduces T = int,
 * and generates (instantiates):
 *
 *     int max_of(int a, int b) { ... }
 *
 * Key points:
 *   - Templates are resolved at COMPILE time -> zero runtime overhead.
 *   - No code is generated for a template until it is used (instantiated).
 *   - `typename` and `class` mean the same thing inside template <...>.
 *   - Compare with C: in C you'd write max_int, max_double, ... or use an
 *     unsafe macro like #define MAX(a,b) ((a)>(b)?(a):(b)).
 *
 * Build:  g++ -std=c++17 -Wall -Wextra 01_function_template.cpp -o func_tmpl
 */

#include <iostream>
#include <string>
#include <cstring>

/* ------------------------------------------------------------------ */
/* 1. Basic function template                                          */
/* ------------------------------------------------------------------ */
/*
 * Works for any T that supports operator '>'.
 * Arguments taken by const reference to avoid copying big objects
 * (e.g. std::string).
 */
template <typename T>
const T& max_of(const T& a, const T& b)
{
    return (a > b) ? a : b;
}

/* ------------------------------------------------------------------ */
/* 2. Multiple template parameters                                     */
/* ------------------------------------------------------------------ */
/*
 * T and U can be different types. The return type is deduced with
 * `auto` (C++14), so add(1, 2.5) returns double, not int.
 */
template <typename T, typename U>
auto add(T a, U b)
{
    return a + b;
}

/* ------------------------------------------------------------------ */
/* 3. Non-type template parameter                                      */
/* ------------------------------------------------------------------ */
/*
 * A template parameter can also be a compile-time VALUE (int, size_t,
 * pointer, enum...). Here N is the array size, deduced automatically
 * from the array we pass - no need to pass the length separately like in C.
 */
template <typename T, std::size_t N>
void print_array(const T (&arr)[N])
{
    std::cout << "[ ";
    for (std::size_t i = 0; i < N; ++i)
        std::cout << arr[i] << ' ';
    std::cout << "]  (N = " << N << ")\n";
}

/* ------------------------------------------------------------------ */
/* 4. Generic swap                                                     */
/* ------------------------------------------------------------------ */
template <typename T>
void my_swap(T& a, T& b)
{
    T tmp = a;      /* requires T to be copyable */
    a = b;
    b = tmp;
}

/* ------------------------------------------------------------------ */
/* 5. Full (explicit) specialization                                   */
/* ------------------------------------------------------------------ */
/*
 * Problem: max_of("apple", "mango") with T = const char* compares the
 * POINTER addresses, not the strings! We provide a special version
 * for const char* that uses strcmp instead.
 *
 * Syntax: template <> followed by the function with T replaced.
 */
template <>
const char* const& max_of<const char*>(const char* const& a, const char* const& b)
{
    return (std::strcmp(a, b) > 0) ? a : b;
}

/* ------------------------------------------------------------------ */
/* 6. Overloading a template with a normal function                    */
/* ------------------------------------------------------------------ */
/*
 * If a non-template function matches exactly, the compiler PREFERS it
 * over the template.
 */
template <typename T>
void describe(T)
{
    std::cout << "describe<T>: generic template version\n";
}

void describe(int)
{
    std::cout << "describe(int): plain non-template overload\n";
}

int main()
{
    std::cout << "=== 1. Basic template / type deduction ===\n";
    std::cout << "max_of(3, 7)          = " << max_of(3, 7) << '\n';          /* T = int    */
    std::cout << "max_of(2.5, 1.5)      = " << max_of(2.5, 1.5) << '\n';      /* T = double */
    std::cout << "max_of('a', 'z')      = " << max_of('a', 'z') << '\n';      /* T = char   */

    std::string s1 = "hello", s2 = "world";
    std::cout << "max_of(string,string) = " << max_of(s1, s2) << '\n';        /* T = std::string */

    /*
     * max_of(3, 7.5) would NOT compile: T can't be both int and double.
     * Fix: specify T explicitly -> the int is converted to double.
     */
    std::cout << "max_of<double>(3, 7.5)= " << max_of<double>(3, 7.5) << '\n';

    std::cout << "\n=== 2. Multiple type parameters ===\n";
    std::cout << "add(1, 2.5)   = " << add(1, 2.5) << '\n';    /* T=int, U=double -> double */
    std::cout << "add(10, 'A')  = " << add(10, 'A') << '\n';   /* 10 + 65 = 75 (char promoted) */

    std::cout << "\n=== 3. Non-type parameter (array size) ===\n";
    int    nums[]   = {1, 2, 3, 4, 5};
    double reals[]  = {1.1, 2.2, 3.3};
    print_array(nums);      /* T = int,    N = 5 */
    print_array(reals);     /* T = double, N = 3 */

    std::cout << "\n=== 4. Generic swap ===\n";
    int x = 10, y = 20;
    my_swap(x, y);
    std::cout << "after swap: x = " << x << ", y = " << y << '\n';
    my_swap(s1, s2);
    std::cout << "after swap: s1 = " << s1 << ", s2 = " << s2 << '\n';

    std::cout << "\n=== 5. Specialization for const char* ===\n";
    const char* a = "apple";
    const char* b = "mango";
    std::cout << "max_of(\"apple\", \"mango\") = " << max_of(a, b) << '\n';

    std::cout << "\n=== 6. Template vs non-template overload ===\n";
    describe(42);      /* exact match -> non-template wins      */
    describe(3.14);    /* no double overload -> template used   */
    describe<int>(42); /* explicit <int> forces template version */

    return 0;
}
