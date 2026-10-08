#ifndef PDB4_H
#define PDB4_H

#include <stdint.h>


enum {
    PDB4_CUBIES = 4,
    PDB4_BLANK = 4,
    PDB4_ORIENTATIONS = 81,
    PDB4_ENTRIES = 840 * PDB4_ORIENTATIONS,
    PDB4_BYTES = (PDB4_ENTRIES + 1) / 2
};
/* Each destination takes a cubie from source[face][destination]. */
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
/* Displayed IDs {2,3,6,7}; solver IDs are zero-based. */
static const uint8_t pdb4_selected[4] = {1, 2, 5, 6};

typedef struct {
    uint8_t piece[7]; /* Labels 0..3, or PDB4_BLANK for an ignored cubie. */
    uint8_t twist[7]; /* Blank twists are always zero. */
} pdb4_state;

/* Project a valid full cube into a pattern. */
static inline pdb4_state pdb4_project(const uint8_t p[7], const uint8_t o[7])
{
    pdb4_state result;
    for (unsigned slot = 0; slot < 7; ++slot) {
        result.piece[slot] = PDB4_BLANK;
        result.twist[slot] = 0;
        for (unsigned label = 0; label < PDB4_CUBIES; ++label) {
            if (p[slot] == pdb4_selected[label]) {
                result.piece[slot] = (uint8_t) label;
                result.twist[slot] = o[slot];
                break;
            }
        }
    }
    return result;
}

/* Rank a valid pattern: each selected label appears exactly once. */
static inline uint32_t pdb4_rank(const pdb4_state *s)
{
    uint8_t position[4] = {0};
    uint8_t used[7] = {0};
    uint32_t p = 0, o = 0;

    for (unsigned slot = 0; slot < 7; ++slot) {
        if (s->piece[slot] < PDB4_CUBIES) {
            position[s->piece[slot]] = (uint8_t) slot;
            /* Twist digits follow increasing occupied-slot order. */
            o = o * 3 + s->twist[slot];
        }
    }
    for (unsigned label = 0; label < PDB4_CUBIES; ++label) {
        unsigned digit = 0;
        for (unsigned slot = 0; slot < position[label]; ++slot)
            if (!used[slot])
                ++digit;
        p = p * (7 - label) + digit;
        used[position[label]] = 1;
    }
    return p * PDB4_ORIENTATIONS + o;
}

/* Inverse of pdb4_rank; index must be below PDB4_ENTRIES. */
static inline pdb4_state pdb4_unrank(uint32_t index)
{
    uint32_t p = index / PDB4_ORIENTATIONS;
    uint32_t o = index % PDB4_ORIENTATIONS;
    uint8_t digit[4];
    uint8_t available[7] = {0, 1, 2, 3, 4, 5, 6};
    pdb4_state result;

    for (unsigned label = PDB4_CUBIES; label-- > 0;) {
        digit[label] = (uint8_t) (p % (7 - label));
        p /= 7 - label;
    }
    for (unsigned slot = 0; slot < 7; ++slot) {
        result.piece[slot] = PDB4_BLANK;
        result.twist[slot] = 0;
    }
    for (unsigned label = 0; label < PDB4_CUBIES; ++label) {
        unsigned chosen = digit[label];
        result.piece[available[chosen]] = (uint8_t) label;
        for (unsigned i = chosen; i + 1 < 7 - label; ++i)
            available[i] = available[i + 1];
    }
    for (unsigned slot = 7; slot-- > 0;) {
        if (result.piece[slot] < PDB4_CUBIES) {
            result.twist[slot] = (uint8_t) (o % 3);
            o /= 3;
        }
    }
    return result;
}

static inline pdb4_state pdb4_quarter_turn(pdb4_state s, uint8_t face)
{
    pdb4_state result;
    for (unsigned slot = 0; slot < 7; ++slot) {
        unsigned from = source[face][slot];
        result.piece[slot] = s.piece[from];
        result.twist[slot] = result.piece[slot] == PDB4_BLANK ? 0 :
            (uint8_t) ((s.twist[from] + twist[face][slot]) % 3);
    }
    return result;
}

#endif
