#include <stdio.h>
int main(void)
{
    //float, double, and long double.

    float f = 0.314f;         // suffix f or F denotes type float
    double d = 0.314;         // no suffix denotes double
    long double ld = 0.314l;  // suffix l or L denotes long double
    
    // Optional components in floating-point definitions
    double x = 1.;            // valid, fractional part is optional
    double y = .1;            // valid, whole-number part is optional
    
    // Scientific notation examples
    double sd = 1.2e3;        // 1.2 scaled by 10^3 (1200.0)

    // Print all floating-point values
    printf("\033[1;34m// FLOATING-POINT CONSTANTS DEMO\033[0m\n\n");

    printf("\033[1;33m[Single Precision Float]\033[0m\n");
    printf("  f = %.6f (float)\n", f);

    printf("\n\033[1;35m[Double Precision Float]\033[0m\n");
    printf("  d  = %.6f (double)\n", d);
    printf("  x  = %.1f (double, fractional part optional)\n", x);
    printf("  y  = %.1f (double, whole-number part optional)\n", y);
    printf("  sd = %.1e (scientific notation, double)\n", sd);

    printf("\n\033[1;36m[Extended Precision Float]\033[0m\n");
    printf("  ld = %.10Lf (long double)\n", ld);

    printf("\033[1;31m\n// GGWP: Floating-point mastery unlocked!\033[0m\n");

    return 0; // All constants successfully printed

    return 0;
}