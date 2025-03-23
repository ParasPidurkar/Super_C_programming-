#include <stdio.h>

int main()
{
    int foo = 20; // Start at 20 ??/
    int bar = 0;
    // The following will cause a compilation error (undeclared variable 'bar')
    // because 'int bar = 0;' is part of the comment on the preceding line
    bar += foo;
    return 0;
}