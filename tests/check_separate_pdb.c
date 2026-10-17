/* AI-assisted host checks; the full BFS oracle never enters the target. */
#define main solver_cli_main
#include "../solver.c"
#undef main
#include <assert.h>

#ifdef NDEBUG
#error "Run these checks with assertions enabled"
#endif

int main(void)
{
    assert(sizeof permutation_distances == PERMUTATIONS);
    assert(sizeof orientation_distances == ORIENTATIONS);
    assert(permutation_distances[0] == 0 && orientation_distances[0] == 0);
    build_moves();
    uint8_t *distance = malloc(STATES);
    uint32_t *queue = malloc((size_t) STATES * sizeof *queue);
    assert(distance && queue);
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
                    assert(tail < STATES);
                    queue[tail++] = next;
                }
            }
        }
    }
    assert(tail == STATES);
    uint8_t min_perm[PERMUTATIONS], min_ori[ORIENTATIONS];
    memset(min_perm, 255, sizeof min_perm);
    memset(min_ori, 255, sizeof min_ori);
    for (uint32_t rank = 0; rank < STATES; ++rank) {
        unsigned p = rank / ORIENTATIONS, o = rank % ORIENTATIONS;
        unsigned hp = permutation_distances[p], ho = orientation_distances[o];
        assert(hp <= distance[rank] && ho <= distance[rank]);
        assert(heuristic(p, o) == (hp > ho ? hp : ho));
        if (distance[rank] < min_perm[p]) min_perm[p] = distance[rank];
        if (distance[rank] < min_ori[o]) min_ori[o] = distance[rank];
    }
    assert(memcmp(min_perm, permutation_distances, sizeof min_perm) == 0);
    assert(memcmp(min_ori, orientation_distances, sizeof min_ori) == 0);
    puts("PASS: all 5769 generated distances equal full-BFS projection minima");
    puts("PASS H1: separate heuristic admissible on all 3674160 states");
    fflush(stdout);

    unsigned checked = 0, hard = 0;
    for (uint32_t rank = 0; rank < STATES; ++rank) {
        if (distance[rank] != 11 && rank % 7919 != 0)
            continue;
        uint8_t path[MAX_DEPTH];
        int length = solve_ida(rank / ORIENTATIONS, rank % ORIENTATIONS, path);
        assert(length == distance[rank]);
        state_t state;
        unrank_state(rank, &state);
        for (int i = 0; i < length; ++i)
            state = apply_move(state, path[i]);
        assert(rank_state(&state) == 0);
        ++checked;
        hard += distance[rank] == 11;
    }
    assert(hard == 2644);
    printf("PASS: %u optimal solutions replayed, including all %u distance-11 states\n", checked, hard);
    puts("Full H3 over all 3674160 search inputs was not run");
    free(queue);
    free(distance);
    return output_failed() ? 1 : 0;
}
