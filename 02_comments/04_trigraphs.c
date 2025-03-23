#include <stdio.h>

/*
 * Trigraphs in C are sequences of three characters starting with "??".
 * They are replaced by specific single characters during preprocessing.
 * This feature was introduced for keyboards that don't have certain symbols.
 * Below are some examples of trigraphs and their replacements:
 *
 * Trigraph   Replacement
 * ??=        #
 * ??(        [
 * ??)        ]
 * ??!        |
 * ??<        {
 * ??>        }
 * ??'        ^
 * ??-        ~
 * ??/        \
 */

int main() {
    // Using trigraphs in code
    printf("Hello, world!\n");

    // Example: Replacing ??< and ??> with { and }
    ??<  // Equivalent to {
        printf("This block uses trigraphs for curly braces!\n");
    ??>  // Equivalent to }

    // Example: Using ??= for #
    ??=include <stdio.h>  // Equivalent to #include <stdio.h> (not recommended in modern code)

    return 0;
}
