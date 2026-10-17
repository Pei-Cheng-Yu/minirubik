#include <stdio.h>
#include "separate_pdb_data.h"

static void emit(const char *name, const uint8_t *distance, size_t count)
{
    printf(".globl %s\n%s:\n", name, name);
    for (size_t i = 0; i < count; ++i)
        printf("    .byte %u\n", (unsigned) distance[i]);
}

int main(void)
{
    puts(".section .rodata");
    emit("permutation_distances", permutation_distances,
         sizeof permutation_distances);
    emit("orientation_distances", orientation_distances,
         sizeof orientation_distances);

    return fflush(stdout) != 0 || ferror(stdout);
}
