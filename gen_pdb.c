/* AI-assisted host generator. Only the emitted distance arrays enter the solver. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Destination slot takes its cubie from source[face][slot]. Faces: R, B, D. */
static const uint8_t source[3][7] = {
    {1, 4, 2, 0, 3, 5, 6},
    {0, 1, 2, 4, 5, 6, 3},
    {0, 2, 5, 3, 1, 4, 6},
};
static const uint8_t twist[3][7] = {
    {1, 2, 0, 2, 1, 0, 0},
    {0, 0, 0, 1, 2, 1, 2},
    {0, 0, 0, 0, 0, 0, 0},
};

#ifdef NDEBUG
#error "Build the generator with assertions enabled"
#endif

enum { PERMS = 5040, ORIS = 729 };
static uint16_t perm_moves[3 * PERMS], ori_moves[3 * ORIS];
static uint8_t perm_dist[PERMS], ori_dist[ORIS];

/* Decode one projection, matching the solver's rank order. */
static void decode(unsigned rank, int orientation, uint8_t values[7])
{
    if (orientation) {
        unsigned sum = 0;
        for (int i = 5; i >= 0; --i) {
            values[i] = (uint8_t) (rank % 3);
            sum += values[i];
            rank /= 3;
        }
        values[6] = (uint8_t) ((3 - sum % 3) % 3);
    } else {
        static const unsigned weights[7] = {720, 120, 24, 6, 2, 1, 1};
        uint8_t available[7] = {0, 1, 2, 3, 4, 5, 6};
        for (unsigned i = 0; i < 7; ++i) {
            unsigned q = rank / weights[i];
            rank %= weights[i];
            values[i] = available[q];
            for (unsigned j = q; j + 1 < 7 - i; ++j)
                available[j] = available[j + 1];
        }
    }
}

static unsigned encode(const uint8_t values[7], int orientation)
{
    unsigned rank = 0;
    if (orientation) {
        for (unsigned i = 0; i < 6; ++i)
            rank = rank * 3 + values[i];
    } else {
        for (unsigned i = 0; i < 7; ++i) {
            unsigned smaller = 0;
            for (unsigned j = i + 1; j < 7; ++j)
                smaller += values[j] < values[i];
            rank = rank * (7 - i) + smaller;
        }
    }
    return rank;
}

static void build(unsigned count, int orientation, uint16_t *moves, uint8_t *dist)
{
    for (unsigned rank = 0; rank < count; ++rank) {
        uint8_t values[7], next[7];
        decode(rank, orientation, values);
        assert(encode(values, orientation) == rank);
        for (unsigned face = 0; face < 3; ++face) {
            for (unsigned slot = 0; slot < 7; ++slot) {
                unsigned value = values[source[face][slot]];
                if (orientation)
                    value = (value + twist[face][slot]) % 3;
                next[slot] = (uint8_t) value;
            }
            moves[face * count + rank] = (uint16_t) encode(next, orientation);
        }
    }

    uint16_t queue[PERMS];
    unsigned head = 0, tail = 1;
    memset(dist, 255, count);
    dist[0] = 0;
    queue[0] = 0;
    while (head < tail) {
        unsigned here = queue[head++];
        for (unsigned face = 0; face < 3; ++face) {
            unsigned next = here;
            for (unsigned turn = 0; turn < 3; ++turn) {
                next = moves[face * count + next];
                if (dist[next] == 255) {
                    /* Quarter, half, inverse all cost one move from here. */
                    dist[next] = (uint8_t) (dist[here] + 1);
                    assert(tail < count);
                    queue[tail++] = (uint16_t) next;
                }
            }
        }
    }
    assert(tail == count);
}

static void check(unsigned count, const uint16_t *moves, const uint8_t *dist,
                  unsigned expected_max)
{
    unsigned maximum = 0;
    for (unsigned here = 0; here < count; ++here) {
        assert(dist[here] != 255 && ((dist[here] == 0) == (here == 0)));
        if (dist[here] > maximum)
            maximum = dist[here];
        int decreases = here == 0;
        for (unsigned face = 0; face < 3; ++face) {
            unsigned next = here;
            for (unsigned turn = 0; turn < 3; ++turn) {
                next = moves[face * count + next];
                assert(dist[here] <= dist[next] + 1 && dist[next] <= dist[here] + 1);
                decreases |= dist[next] + 1 == dist[here];
            }
            assert(moves[face * count + next] == here);
        }
        assert(decreases);
    }
    assert(maximum == expected_max);
}

static void emit(const char *name, const uint8_t *dist, unsigned count)
{
    printf("static const uint8_t %s[%u] = {\n", name, count);
    for (unsigned i = 0; i < count; ++i) {
        printf("%u,", dist[i]);
        if (i % 24 == 23)
            putchar('\n');
    }
    puts("\n};");
}

int main(int argc, char **argv)
{
    int self_test = argc == 2 && strcmp(argv[1], "--self-test") == 0;
    if (argc != 1 && !self_test) {
        fputs("usage: gen_pdb [--self-test]\n", stderr);
        return 2;
    }
    build(PERMS, 0, perm_moves, perm_dist);
    build(ORIS, 1, ori_moves, ori_dist);
    check(PERMS, perm_moves, perm_dist, 7);
    check(ORIS, ori_moves, ori_dist, 6);
    if (self_test) {
        puts("PASS: 5769 projection round trips; complete exact tables; maxima 7 and 6; solved entries zero");
    } else {
        puts("/* Generated by gen_pdb.c; byte distances in the baseline rank order. */");
        puts("#ifndef SEPARATE_PDB_DATA_H\n#define SEPARATE_PDB_DATA_H\n#include <stdint.h>");
        emit("permutation_distances", perm_dist, PERMS);
        emit("orientation_distances", ori_dist, ORIS);
        puts("#endif");
        fprintf(stderr, "Separate PDBs: %u + %u = %u bytes\n", PERMS, ORIS, PERMS + ORIS);
    }
    return fflush(stdout) != 0 || ferror(stdout);
}
