/* AI-assisted host diagnostic: count evaluations, not machine instructions. */
#define COUNT_HEURISTIC
#define main solver_cli_main
#include "../solver.c"
#undef main

int main(int argc, char **argv)
{
    if (argc < 2) {
        fputs("usage: count_heuristic STATE [STATE ...]\n", stderr);
        return 2;
    }
    /* Validate all inputs before starting the experiment. */
    for (int i = 1; i < argc; ++i) {
        state_t state;
        if (!parse_state(argv[i], &state)) {
            fprintf(stderr, "invalid state: %s\n", argv[i]);
            return 2;
        }
    }

    build_moves();
    puts("input,solution_length,heuristic_calls");
    for (int i = 1; i < argc; ++i) {
        state_t state;
        parse_state(argv[i], &state);
        uint32_t rank = rank_state(&state);
        uint8_t solution[MAX_DEPTH];

        heuristic_calls = 0;
        int length = solve_ida(rank / ORIENTATIONS, rank % ORIENTATIONS,
                               solution);
        if (length < 0 || length > MAX_DEPTH) {
            fprintf(stderr, "search failed: %s\n", argv[i]);
            return 1;
        }
        for (int j = 0; j < length; ++j)
            state = apply_move(state, solution[j]);
        if (rank_state(&state) != 0) {
            fprintf(stderr, "solution replay failed: %s\n", argv[i]);
            return 1;
        }
        printf("%s,%d,%llu\n", argv[i], length, heuristic_calls);
    }
    return output_failed() ? 1 : 0;
}
