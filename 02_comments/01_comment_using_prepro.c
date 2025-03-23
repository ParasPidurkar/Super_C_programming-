#include <stdio.h>

int main()
{

// single line comment 

/*multi line comment*/

/*
 * this is a
 * multi-line
 * comment
*/


#if 0 /* Starts the "comment", anything from here on is removed by preprocessor */
    puts("want to be the very best,Like no one ever was.");
    puts("To catch 'em is my real test,To train them is my cause."); 
#endif /* 0 */

/* Code from here on is "uncommented" (included in compiled executable) */
    puts(" Pokémon! (Gotta catch 'em all!) It's you and me,");
    puts(" I know it's my destiny!"); 
}