/* AI-assisted host test utility: enumerate states at an exact BFS distance.
 * Reuse the cube model; do not run IDA* or use PDB distances as the answer.
 */
#define main solver_cli_main
#include "../solver.c"
#undef main

int main(int argc, char **argv)
{
    long target = 11;
    if (argc == 2) {
        char *end;
        target = strtol(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || target < 0 || target > 11) {
            fputs("distance must be an integer from 0 to 11\n", stderr);
            return 2;
        }
    } else if (argc != 1) {
        fputs("usage: gen_states [distance: 0..11; default 11]\n", stderr);
        return 2;
    }

    build_moves();
    uint8_t *distance = malloc(STATES);
    uint32_t *queue = malloc((size_t) STATES * sizeof *queue);
    if (!distance || !queue) {
        fputs("cannot allocate host BFS storage\n", stderr);
        free(distance);
        free(queue);
        return 1;
    }
    memset(distance, 255, STATES);
    distance[0] = 0;
    queue[0] = 0;
    uint32_t head = 0, tail = 1;

    while (head < tail) {
        uint32_t rank = queue[head++];
        for (unsigned face = 0; face < 3; ++face) {
            uint16_t p = rank / ORIENTATIONS, o = rank % ORIENTATIONS;
            for (unsigned turn = 0; turn < 3; ++turn) {
                p = permutation[face][p];
                o = orientation[face][o];
                uint32_t next = (uint32_t) p * ORIENTATIONS + o;
                if (distance[next] == 255) {
                    distance[next] = (uint8_t) (distance[rank] + 1);
                    queue[tail++] = next;
                }
            }
        }
    }
    if (tail != STATES) {
        fputs("BFS did not reach every cube state\n", stderr);
        free(distance);
        free(queue);
        return 1;
    }

    unsigned emitted = 0, diameter = 0;
    puts("rank,input,distance");
    for (uint32_t rank = 0; rank < STATES; ++rank) {
        if (distance[rank] > diameter)
            diameter = distance[rank];
        if (distance[rank] != target)
            continue;
        state_t state;
        unrank_state(rank, &state);
        if (!valid(&state) || rank_state(&state) != rank) {
            fputs("invalid state or rank round-trip failure\n", stderr);
            free(distance);
            free(queue);
            return 1;
        }
        printf("%u,", (unsigned) rank);
        for (unsigned i = 0; i < CUBIES; ++i)
            putchar('1' + state.p[i]);
        for (unsigned i = 0; i < CUBIES; ++i)
            putchar('1' + state.o[i]);
        printf(",%u\n", (unsigned) distance[rank]);
        ++emitted;
    }
    free(distance);
    free(queue);
    if (output_failed())
        return 1;
    fprintf(stderr, "BFS: %u reachable states; diameter %u; emitted %u states at distance %ld\n",
            (unsigned) tail, diameter, emitted, target);
    return 0;
}
