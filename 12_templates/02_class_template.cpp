/*
 * 02_class_template.cpp
 *
 * CLASS TEMPLATES
 * ---------------
 * Same idea as function templates, but for a whole class. You write a
 * container / data structure once and the compiler generates a separate
 * class for every type you use:
 *
 *     Stack<int>          -> a class that stores ints
 *     Stack<std::string>  -> a completely different class storing strings
 *
 * Stack<int> and Stack<double> are DIFFERENT, UNRELATED types.
 *
 * Covered here:
 *   1. A generic Stack<T, Capacity>   (type + non-type parameter)
 *   2. Defining member functions OUTSIDE the class body
 *   3. A class with two type parameters: Pair<K, V>
 *   4. Default template arguments
 *   5. Full specialization:    Box<bool>
 *   6. Partial specialization: Box<T*>
 *   7. A template member function inside a class template
 *   8. Class Template Argument Deduction (CTAD, C++17)
 *
 * NOTE on header files: template code is normally kept entirely in the
 * .h/.hpp file. The compiler must SEE the full definition at the point
 * of use to generate code, so splitting into .h + .cpp (like the C
 * library examples in 07/08) gives "undefined reference" linker errors.
 *
 * Build:  g++ -std=c++17 -Wall -Wextra 02_class_template.cpp -o class_tmpl
 */

#include <iostream>
#include <string>
#include <stdexcept>

/* ------------------------------------------------------------------ */
/* 1. Generic fixed-size stack                                         */
/* ------------------------------------------------------------------ */
/*
 * T        : type of elements
 * Capacity : compile-time size (non-type parameter), default = 10.
 *            The array lives inside the object - no malloc needed.
 */
template <typename T, std::size_t Capacity = 10>
class Stack
{
public:
    void push(const T& value);      /* defined outside the class, below */
    T    pop();
    const T& top() const;

    /* Short functions can be defined inline inside the class body. */
    bool        empty() const { return count == 0; }
    bool        full()  const { return count == Capacity; }
    std::size_t size()  const { return count; }

private:
    T           data[Capacity];
    std::size_t count = 0;
};

/* ------------------------------------------------------------------ */
/* 2. Member functions defined OUTSIDE the class                       */
/* ------------------------------------------------------------------ */
/*
 * You must repeat the template header, and qualify the class name
 * with its parameters: Stack<T, Capacity>::push
 * (Default arguments like "= 10" are NOT repeated here.)
 */
template <typename T, std::size_t Capacity>
void Stack<T, Capacity>::push(const T& value)
{
    if (full())
        throw std::overflow_error("Stack overflow");
    data[count++] = value;
}

template <typename T, std::size_t Capacity>
T Stack<T, Capacity>::pop()
{
    if (empty())
        throw std::underflow_error("Stack underflow");
    return data[--count];
}

template <typename T, std::size_t Capacity>
const T& Stack<T, Capacity>::top() const
{
    if (empty())
        throw std::underflow_error("Stack is empty");
    return data[count - 1];
}

/* ------------------------------------------------------------------ */
/* 3. Two type parameters + 7. template member function                */
/* ------------------------------------------------------------------ */
template <typename K, typename V>
class Pair
{
public:
    Pair(const K& k, const V& v) : key(k), value(v) {}

    const K& get_key()   const { return key; }
    const V& get_value() const { return value; }

    /*
     * A member function can have ITS OWN template parameter (F),
     * independent of K and V. Here F is any callable (lambda,
     * function pointer, functor) that takes a V and returns a V.
     */
    template <typename F>
    void transform_value(F func)
    {
        value = func(value);
    }

    void print() const
    {
        std::cout << "(" << key << ", " << value << ")\n";
    }

private:
    K key;
    V value;
};

/* ------------------------------------------------------------------ */
/* 5 & 6. Primary template + specializations                           */
/* ------------------------------------------------------------------ */
/* Primary (general) template - used for any T not specialized below. */
template <typename T>
class Box
{
public:
    explicit Box(const T& v) : item(v) {}
    void show() const { std::cout << "Box<T>     holds: " << item << '\n'; }
private:
    T item;
};

/*
 * FULL specialization: a completely separate implementation used ONLY
 * when T is exactly bool. Note the empty template <>.
 */
template <>
class Box<bool>
{
public:
    explicit Box(bool v) : item(v) {}
    void show() const
    {
        std::cout << "Box<bool>  holds: " << (item ? "true" : "false") << '\n';
    }
private:
    bool item;
};

/*
 * PARTIAL specialization: used for ANY pointer type T*.
 * Still a template (has a parameter T), but only matches pointers.
 * Here we print the value pointed to, not the address.
 * (Partial specialization is allowed for classes, NOT for functions.)
 */
template <typename T>
class Box<T*>
{
public:
    explicit Box(T* p) : ptr(p) {}
    void show() const
    {
        if (ptr)
            std::cout << "Box<T*>    points to: " << *ptr << '\n';
        else
            std::cout << "Box<T*>    is null\n";
    }
private:
    T* ptr;
};

int main()
{
    std::cout << "=== 1/2. Stack<int, 5> ===\n";
    Stack<int, 5> si;                 /* T = int, Capacity = 5 */
    for (int i = 1; i <= 5; ++i)
        si.push(i * 10);
    std::cout << "size = " << si.size() << ", top = " << si.top() << '\n';

    try {
        si.push(60);                  /* 6th element -> overflow */
    } catch (const std::overflow_error& e) {
        std::cout << "caught: " << e.what() << '\n';
    }

    std::cout << "popping: ";
    while (!si.empty())
        std::cout << si.pop() << ' ';
    std::cout << '\n';

    std::cout << "\n=== 4. Default argument: Stack<std::string> (Capacity = 10) ===\n";
    Stack<std::string> ss;
    ss.push("first");
    ss.push("second");
    std::cout << "top = " << ss.top() << ", size = " << ss.size() << '\n';

    std::cout << "\n=== 3/7. Pair<K, V> + template member function ===\n";
    Pair<std::string, int> age("Paras", 28);
    age.print();
    age.transform_value([](int v) { return v + 1; });   /* F = lambda type */
    std::cout << "after +1: ";
    age.print();

    std::cout << "\n=== 5/6. Specializations of Box ===\n";
    int n = 99;
    Box<double> bd(3.14);   /* primary template      */
    Box<bool>   bb(true);   /* full specialization   */
    Box<int*>   bp(&n);     /* partial specialization */
    bd.show();
    bb.show();
    bp.show();

    std::cout << "\n=== 8. CTAD (C++17) ===\n";
    /*
     * Since C++17 the compiler can deduce class template arguments from
     * the constructor, so we don't have to write Pair<std::string, double>.
     *
     * GOTCHA: Pair p("pi", 3.14) does NOT compile. The constructor takes
     * const K&, and binding a reference to "pi" does not decay the array,
     * so K is deduced as char[3] - and an array member can't be copied.
     * Passing a std::string (or adding a deduction guide) fixes it.
     */
    Pair p(std::string("pi"), 3.14159);   /* deduced: Pair<std::string, double> */
    p.print();

    return 0;
}
