#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "separate_pdb_data.h"

enum {
    CUBIES = 7,
    PERMUTATIONS = 5040,
    ORIENTATIONS = 729,
    STATES = PERMUTATIONS * ORIENTATIONS,
    MOVES = 9
};

typedef struct {
    uint8_t p[CUBIES], o[CUBIES];
} state_t;

/*@ predicate valid_state(state_t *state) =
      (\forall integer i; 0 <= i < CUBIES ==>
         state->p[i] < CUBIES && state->o[i] < 3) &&
      (\forall integer i, j; 0 <= i < j < CUBIES ==>
         state->p[i] != state->p[j]) &&
      (state->o[0] + state->o[1] + state->o[2] + state->o[3] +
       state->o[4] + state->o[5] + state->o[6]) % 3 == 0;
 */

static const char *const move_names[MOVES] = {"R",  "R2", "R'", "B", "B2",
                                              "B'", "D",  "D2", "D'"};
static const uint8_t inverse_move[MOVES] = {2, 1, 0, 5, 4, 3, 8, 7, 6};

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

/* The three quarter-turns preserve the fixed front-upper-left corner. */
/*@ requires face < 3;
    requires \forall integer i; 0 <= i < CUBIES ==> state.o[i] < 3;
    assigns \nothing;
    ensures \forall integer i; 0 <= i < CUBIES ==>
              \result.p[i] == state.p[source[face][i]];
    ensures \forall integer i; 0 <= i < CUBIES ==>
              \result.o[i] == (state.o[source[face][i]] + twist[face][i]) % 3;
 */
static state_t quarter_turn(state_t state, uint8_t face)
{
    state_t result;
    /*@ loop invariant 0 <= i <= CUBIES;
        loop invariant \forall integer j; 0 <= j < i ==>
          result.p[j] == state.p[source[face][j]];
        loop invariant \forall integer j; 0 <= j < i ==>
          result.o[j] == (state.o[source[face][j]] + twist[face][j]) % 3;
        loop assigns i, result.p[0..6], result.o[0..6];
        loop variant CUBIES - i;
    */
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t from = source[face][i];
        result.p[i] = state.p[from];
        unsigned value = state.o[from] + twist[face][i];
        if (value >= 3U)
            value -= 3U;
        result.o[i] = (uint8_t) value;
    }
    return result;
}

static state_t apply_move(state_t state, uint8_t move)
{
    uint8_t face = (uint8_t) ((move >= 3U) + (move >= 6U));
    uint8_t turns = (uint8_t) (move - ((face << 1) + face) + 1U);
    for (uint8_t i = 0; i < turns; ++i)
        state = quarter_turn(state, face);
    return state;
}

/*@ requires \valid_read(state);
    requires \forall integer i; 0 <= i < CUBIES ==>
      0 <= state->p[i] < CUBIES;
    requires \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    requires \forall integer i; 0 <= i < CUBIES ==>
      0 <= state->o[i] < 3;
    requires \valid(permutation_rank) && \valid(orientation_rank);
    requires \separated(state, permutation_rank, orientation_rank);
    assigns *permutation_rank, *orientation_rank;
    ensures *permutation_rank < PERMUTATIONS;
    ensures *orientation_rank < ORIENTATIONS;
 */
static void rank_components(const state_t *state,
                            uint16_t *permutation_rank,
                            uint16_t *orientation_rank)
{
    uint32_t p = 0, o = 0;
    /*@ loop invariant 0 <= i <= CUBIES;
        loop invariant (i == 0 ==> p == 0) && (i == 1 ==> p <= 6) &&
          (i == 2 ==> p <= 41) && (i == 3 ==> p <= 209) &&
          (i == 4 ==> p <= 839) && (i == 5 ==> p <= 2519) &&
          (i >= 6 ==> p <= 5039);
        loop assigns i, p;
        loop variant CUBIES - i;
     */
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t smaller = 0;
        /*@ loop invariant i + 1 <= j <= CUBIES;
            loop invariant smaller <= j - i - 1;
            loop assigns j, smaller;
            loop variant CUBIES - j;
         */
        for (uint8_t j = (uint8_t) (i + 1U); j < CUBIES; ++j)
            if (state->p[j] < state->p[i])
                ++smaller;
        p = p * (CUBIES - i) + smaller;
    }
    /*@ loop invariant 0 <= i <= 6;
        loop invariant (i == 0 ==> o == 0) && (i == 1 ==> o < 3) &&
          (i == 2 ==> o < 9) && (i == 3 ==> o < 27) &&
          (i == 4 ==> o < 81) && (i == 5 ==> o < 243) &&
          (i == 6 ==> o < 729);
        loop assigns i, o;
        loop variant 6 - i;
     */
    for (uint8_t i = 0; i < 6; ++i)
        o = o * 3U + state->o[i];
    *permutation_rank = (uint16_t) p;
    *orientation_rank = (uint16_t) o;
}

/*@ requires \valid_read(state);
    requires \forall integer i; 0 <= i < CUBIES ==> state->p[i] < CUBIES;
    requires \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    requires \forall integer i; 0 <= i < CUBIES ==> state->o[i] < 3;
    assigns \nothing;
    ensures \result < STATES;
 */
static uint32_t rank_state(const state_t *state)
{
    uint16_t p, o;
    rank_components(state, &p, &o);
    return (uint32_t) p * ORIENTATIONS + o;
}

/*@ requires \valid(state); requires rank < STATES; assigns *state; */
static void unrank_state(uint32_t rank, state_t *state)
{
    uint8_t available[CUBIES] = {0, 1, 2, 3, 4, 5, 6};
    uint32_t p = rank / ORIENTATIONS, o = rank % ORIENTATIONS, f = 720;
    uint8_t sum = 0;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t q = (uint8_t) (p / f);
        p %= f;
        state->p[i] = available[q];
        for (uint8_t j = q; j + 1U < CUBIES - i; ++j)
            available[j] = available[j + 1U];
        if (i < 5)
            f /= 6U - i;
    }
    static const uint16_t weights[6] = {243, 81, 27, 9, 3, 1};
    for (uint8_t i = 0; i < 6; ++i) {
        uint32_t weight = weights[i], twice = weight << 1;
        uint8_t digit = 0;
        if (o >= twice) {
            o -= twice;
            digit = 2;
        } else if (o >= weight) {
            o -= weight;
            digit = 1;
        }
        state->o[i] = digit;
        sum = (uint8_t) (sum + digit);
        if (sum >= 3)
            sum = (uint8_t) (sum - 3);
    }
    state->o[6] = (uint8_t) (sum == 0 ? 0 : 3 - sum);
}

/*@ requires \valid_read(state);
    requires \initialized(&state->p[0..6]) && \initialized(&state->o[0..6]);
    assigns \nothing;
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->p[i] < CUBIES && state->o[i] < 3;
    ensures \result != 0 ==> \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    ensures \result != 0 ==>
      (state->o[0] + state->o[1] + state->o[2] + state->o[3] +
       state->o[4] + state->o[5] + state->o[6]) % 3 == 0;
    ensures complete: valid_state(state) ==> \result != 0;
 */
static int valid(const state_t *state)
{
    uint8_t sum = 0;
    /*@ loop invariant 0 <= i <= CUBIES;
        loop invariant sum <= 2 * i;
        loop invariant sum == (i > 0 ? state->o[0] : 0) +
          (i > 1 ? state->o[1] : 0) + (i > 2 ? state->o[2] : 0) +
          (i > 3 ? state->o[3] : 0) + (i > 4 ? state->o[4] : 0) +
          (i > 5 ? state->o[5] : 0) + (i > 6 ? state->o[6] : 0);
        loop invariant \forall integer j; 0 <= j < i ==>
          state->p[j] < CUBIES && state->o[j] < 3;
        loop invariant \forall integer j, k; 0 <= j < k < i ==>
          state->p[j] != state->p[k];
        loop assigns i, sum;
        loop variant CUBIES - i;
    */
    for (uint8_t i = 0; i < CUBIES; ++i) {
        if (state->p[i] >= CUBIES || state->o[i] >= 3)
            return 0;
        /*@ loop invariant 0 <= j <= i;
            loop invariant \forall integer k; 0 <= k < j ==>
              state->p[k] != state->p[i];
            loop assigns j;
            loop variant i - j;
        */
        for (uint8_t j = 0; j < i; ++j)
            if (state->p[j] == state->p[i])
                return 0;
        sum = (uint8_t) (sum + state->o[i]);
    }
    if (sum >= 12U)
        sum -= 12U;
    if (sum >= 6U)
        sum -= 6U;
    if (sum >= 3U)
        sum -= 3U;
    return sum == 0;
}

static uint16_t permutation[3][PERMUTATIONS];
static uint16_t orientation[3][ORIENTATIONS];

enum { MAX_DEPTH = 11, NO_FACE = 3 };

typedef struct {
    uint16_t p, o;
    uint8_t next_move;
    uint8_t previous_face;
    uint8_t incoming_move;
    uint8_t h;
} frame_t;

static frame_t frames[MAX_DEPTH + 1];

static void build_moves(void)
{
    state_t state;
    for (uint16_t p = 0; p < PERMUTATIONS; ++p) {
        unrank_state((uint32_t) p * ORIENTATIONS, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            permutation[face][p] =
                (uint16_t) (rank_state(&next) / ORIENTATIONS);
        }
    }
    for (uint16_t o = 0; o < ORIENTATIONS; ++o) {
        unrank_state(o, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            orientation[face][o] =
                (uint16_t) (rank_state(&next) % ORIENTATIONS);
        }
    }
}
/* Optional host diagnostic; normal builds do not count evaluations. */
#ifdef COUNT_HEURISTIC
static unsigned long long heuristic_calls;
#endif

static uint8_t heuristic(uint16_t p, uint16_t o)
{
#ifdef COUNT_HEURISTIC
    ++heuristic_calls;
#endif
    uint8_t hp = permutation_distances[p];
    uint8_t ho = orientation_distances[o];
    return hp > ho ? hp : ho;
}

static int solve_ida(uint16_t start_p, uint16_t start_o,
                     uint8_t solution[MAX_DEPTH])
{
    uint8_t start_h = heuristic(start_p, start_o);
    unsigned bound = start_h;
    while (bound <= MAX_DEPTH) {
        unsigned next_bound = UINT32_MAX;
        unsigned depth = 0;
        frames[0] = (frame_t) {start_p, start_o, 0, NO_FACE, 0, start_h};

        for (;;) {
            frame_t *current = &frames[depth];
            /* Check a new frame once; resumed parents select their next move. */
            if (current->next_move == 0) {
                unsigned f = depth + current->h;
                if (f > bound) {
                    if (f < next_bound)
                        next_bound = f;
                    if (depth == 0)
                        break;
                    --depth;
                    continue;
                }

                if (current->p == 0 && current->o == 0) {
                    for (unsigned i = 1; i <= depth; ++i)
                        solution[i - 1] = frames[i].incoming_move;
                    return (int) depth;
                }

                /* No child frame exists beyond MAX_DEPTH. */
                if (depth == MAX_DEPTH) {
                    --depth;
                    continue;
                }
            }

            if (current->next_move == MOVES) {
                if (depth == 0)
                    break;
                --depth;
                continue;
            }

            uint8_t move = current->next_move;
            uint8_t face = (uint8_t) ((move >= 3U) + (move >= 6U));
            if (face == current->previous_face) {
                current->next_move = (uint8_t) (move + 3U);
                continue;
            }
            ++current->next_move;
            uint8_t remainder = (uint8_t) (move - ((face << 1) + face));

            frame_t *child = &frames[depth + 1];
            /* R2/R' continue from the preceding sibling's coordinates. */
            uint16_t p = remainder == 0 ? current->p : child->p;
            uint16_t o = remainder == 0 ? current->o : child->o;
            p = permutation[face][p];
            o = orientation[face][o];

            *child = (frame_t) {p, o, 0, face, move, heuristic(p, o)};
            ++depth;
        }

        if (next_bound == UINT32_MAX)
            return -1;
        bound = next_bound;
    }
    return -1;
}

/*@ requires valid_read_string(input);
    requires \valid(state);
    assigns state->p[0..6], state->o[0..6];
    ensures \result != 0 ==> input[14] == '\0';
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->p[i] < CUBIES && state->o[i] < 3;
    ensures \result != 0 ==> \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    ensures \result != 0 ==>
      (state->o[0] + state->o[1] + state->o[2] + state->o[3] +
       state->o[4] + state->o[5] + state->o[6]) % 3 == 0;
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->p[i] == input[i] - '1';
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->o[i] == input[i + CUBIES] - '1';
 */
static int parse_state(const char *input, state_t *state)
{
    /*@ loop invariant 0 <= i <= 14;
        loop invariant i <= strlen(input);
        loop invariant i <= 7 ==> \initialized(&state->p[0..i-1]);
        loop invariant i >= 7 ==> \initialized(&state->p[0..6]);
        loop invariant i >= 7 ==> \initialized(&state->o[0..i-8]);
        loop invariant \forall integer j; 0 <= j < i && j < CUBIES ==>
          state->p[j] == input[j] - '1';
        loop invariant \forall integer j; 0 <= j < i - CUBIES ==>
          state->o[j] == input[j + CUBIES] - '1';
        loop assigns i, state->p[0..6], state->o[0..6];
        loop variant 14 - i;
     */
    for (int i = 0; i < 14; ++i) {
        int limit = i < 7 ? 7 : 3;
        if (input[i] < '1' || input[i] > '0' + limit)
            return 0;
        int index = i < 7 ? i : i - 7;
        (i < 7 ? state->p : state->o)[index] = (uint8_t) (input[i] - '1');
    }
    return input[14] == '\0' && valid(state);
}

/* stdout is fully buffered off a terminal, so a write error surfaces at the
 * flush, not at the printf that queued the bytes. Every exit path that has
 * produced output goes through here.
 */
static int output_failed(void)
{
    return fflush(stdout) != 0 || ferror(stdout);
}

static int self_test(void)
{
    const state_t solved = {{0, 1, 2, 3, 4, 5, 6}, {0}};
    state_t state;
    for (uint8_t move = 0; move < MOVES; ++move) {
        state = solved;
        state = apply_move(state, move);
        state = apply_move(state, inverse_move[move]);
        if (memcmp(&solved, &state, sizeof solved))
            return 0;
    }
    for (uint32_t rank = 0; rank < STATES; ++rank) {
        unrank_state(rank, &state);
        if (!valid(&state) || rank_state(&state) != rank)
            return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    state_t state;
    
    if (argc == 2 && !strcmp(argv[1], "--self-test")) {
        if (!self_test()) {
            fputs("self-test failed\n", stderr);
            return 1;
        }
        puts("3674160 rank/unrank checks; move inverses passed");
        return output_failed();
    }

    if (argc != 2 || !parse_state(argv[1], &state)) {
        /* C99 5.1.2.2.1 lets argv[0] be null when argc is 0. */
        fprintf(stderr, "usage: %s PPPPPPPOOOOOOO\n",
                argc > 0 && argv[0] ? argv[0] : "solver");
        return 2;
    }

    build_moves();

    uint16_t p, o;
    rank_components(&state, &p, &o);
    uint8_t solution[MAX_DEPTH];
    int length = solve_ida(p, o, solution);

    if (length < 0) {
        fputs("could not find a solution\n", stderr);
        return 1;
    }

    const char *separator = "";
    for (int i = 0; i < length; ++i) {
        printf("%s%s", separator, move_names[solution[i]]);
        separator = " ";
    }

    putchar('\n');
    return output_failed();
}
