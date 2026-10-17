/* Reuse solver.c, giving its existing main a different name. */
#define main solver_cli_main
#include "solver.c"
#undef main

int main(void)
{
    build_moves();

    puts(".section .rodata");
    puts(".balign 2");
    puts(".globl permutation_moves");
    puts(".globl orientation_moves");

    puts("permutation_moves:");
    for (unsigned face = 0; face < 3; ++face) {
        for (unsigned p = 0; p < PERMUTATIONS; ++p) {
            printf("    .half %u\n",
                   (unsigned)permutation[face][p]);
        }
    }

    puts("orientation_moves:");
    for (unsigned face = 0; face < 3; ++face) {
        for (unsigned o = 0; o < ORIENTATIONS; ++o) {
            printf("    .half %u\n",
                   (unsigned)orientation[face][o]);
        }
    }

    return fflush(stdout) == EOF || ferror(stdout);
}