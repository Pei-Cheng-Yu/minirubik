# minirubik

An optimal C99 and RV32I solver for the 2×2×2 Rubik’s Cube. The current solver
uses IDA* with separate permutation and orientation pattern databases. The
state space contains 3,674,160 states, with an optimal distance of at most 11
half-turn-metric moves. The RV32I version also supports LED animation in Ripes.

## Why a cube is a graph

Ernő Rubik created the original cube in 1974 to demonstrate how parts can move
independently without breaking the whole. A 3×3 cube has 20 moving pieces and
about 4.3 × 10¹⁹ reachable arrangements. The smaller 2×2 cube keeps the eight
corners and removes the edges and fixed centers. [Philo Li’s formula-free
introduction](https://philoli.com/zh/blog/solve-rubiks-cube-without-formulas/)
offers the key intuition: every turn is reversible, turns can be composed, and
their order matters—`R U` is generally not `U R`.

Human solvers use those facts to move a few pieces while restoring the rest;
the commutator `A B A⁻¹ B⁻¹` is the standard example. This program uses the
same group structure differently: it treats every valid arrangement as a node
and every face turn as an edge. It does not use
the article’s 3×3 Roux stages or a library of memorized algorithms.

The solver gives the eight corner positions the numbers `0–7`. The 2.5D
walkthrough below shows where those numbers are on the physical cube.

## How it works

1. Fix one corner to remove whole-cube rotations.
2. Rank the remaining corner permutation and six independent orientations into
   separate integer coordinates.
3. Build permutation-only and orientation-only distance tables on the host.
4. Search with IDA*, using the maximum of the two distances as the heuristic.
   Moves are `R`, `B`, and `D`, including inverse and half turns.

## Build and run

```sh
make all
make check
make prove   # optional: Frama-C WP proof, needs frama-c and alt-ergo
./solver 21345671111111
```

`make all` builds two binaries. `solver` uses IDA*, with a `--self-test` mode
and diagnostics on stderr. `mini` retains the original golfed BFS algorithm;
it has no `--self-test`. Both return shortest solutions, but their move
sequences can differ.

The 14-digit argument describes the scramble and the printed line is the
solution. Both formats are explained below.

### Ripes: LED animation

Use the Ripes continuous build with the 32-bit ISA simulator. From the repository
root in Linux or WSL, with Make, a host C compiler, and Python 3.10 or newer:

```sh
make rv32-gui
```

Open `build/rv32/solver_gui.s` as assembly source in Ripes. This generated file
combines `solver.s`, `moves_data.s`, and `separate_pdb_data.s`, includes the
renderer, and adapts directives for Ripes's assembler. It does not use `link.ld`.
Edit `cube_input` in `solver.s` to select the input, then regenerate and reopen
the generated file. Keep editing the original source, not the generated file.

1. Add LED Matrix 0 in the I/O tab, with width 35 and height 25.
2. Select the 32-bit ISA simulator and disable ISA extensions.
3. Rebuild with Ctrl+B after adding or changing the peripheral.
4. Pop out the LED display, reset, then press the `>>` Run button.

The program displays the input, searches, prints the solution, and redraws after
each complete move. `R2` counts as one move and produces one redraw. The final
cube should have uniform faces: U white, L orange, F green, R red, B blue,
D yellow. Output `-1` means search failure; `-2` means replay did not reach solved.

For example, input `62345713133111` has the eight-move solution
`R2 D B2 R' D R' D B'`, with nine displayed states including the input.

If Ripes is on PATH, `make run-rv32-gui` also opens the generated source.
Otherwise, supply its local executable path using `RIPES="/path/to/Ripes"`.
No personal installation path is stored in the build scripts. See
[tools/README.md](tools/README.md#ripes-gui-animation) for PowerShell and WSL options.

### Ripes: CLI instruction measurement

Install GNU RISC-V binutils and leave `.equ RENDER, 0` in `solver.s`:

```sh
make rv32-size
Ripes --mode cli --src build/rv32/solver.elf -t elf --proc RV32_ISS --iret --runinfo
```

The default cross-tool prefix is `riscv64-unknown-elf-`; override `CROSS` if
needed, for example `make rv32-size CROSS=riscv32-unknown-elf-`. Run the CLI
command with the Ripes executable installed on that machine.

This route uses GNU assembly and `link.ld` to produce `solver.elf`. It excludes
LED drawing and animation delays; coordinate replay and the final solved check
remain enabled. `--iret` counts the whole executed program, including input
parsing, search, printing, and replay. Code size is the linked `.text` size;
static data is `.data` plus `.bss` plus `.rodata`. Use the same input and Ripes
build when comparing measurements. These commands run one input, not every state.

Commit the sources, table exports, and build scripts. Files under `build/`,
including `solver_gui.s` and `solver.elf`, are regenerated and ignored by Git.

### Reading the 14-digit input

The program receives one 14-digit code with no spaces. For explanation, split
it into two groups:

```diagram
2134567 1111111
└── P ─┘ └── O ─┘
  cubies   twists
```

Imagine seven numbered seats and seven students. A position is a seat fixed in
space; a cubie is the physical corner that can move to another seat. In the
solved cube, cubie 1 sits in position 1, cubie 2 in position 2, and so on.
The real cube has no printed numbers; `0–7` are labels used only by this solver.

#### Step 1: Hold the cube in one direction

Keep `FRONT` facing you and `UP` pointing upward. Position `0` is the corner
nearest the upper-left of the front face. It is an anchor for describing the
other corners; the physical cubie is not glued in place.

```diagram
                              BACK
                    ·───────────────·
                   ╱               ╱│
                  ╱        UP     ╱ │
                 ╱               ╱  │
              [0]───────────────·   │
               │                │   │
               │     FRONT      │ R │
               │                │   ·
               │                │  ╱
               │                │ ╱
               │                │╱
               ·────────────────·
```

`R` marks the narrow `RIGHT` face.

#### Step 2: Separate the front and back layers

A 2×2×2 cube has only corner cubies. Looking from the fixed direction, four
corner positions touch the front face and four touch the back face. Each
bracketed number below names one whole corner, not one colored sticker:

```diagram
 FRONT LAYER                          BACK LAYER

 upper-left   upper-right             upper-left   upper-right
     [0]────────[1]                       [7]────────[4]
      │          │                         │          │
      │          │       front ↔ back      │          │
     [3]────────[2]                       [6]────────[5]
 down-left    down-right               down-left    down-right
```

The front layer runs clockwise from its upper-left corner as `0, 1, 2, 3`.
The back layer is drawn as if seen through the cube from the front: `7` is
upper-left, followed clockwise by `4, 5, 6`.

#### Step 3: Join the two layers into positions 0–7

Slide the back square up and to the right, the same direction the cube recedes
in Step 1, to get the complete 2.5D position map. The back edges are drawn
through the front face rather than hidden behind it:

```diagram
                           BACK
                      [7]────────[4]
                     ╱ │        ╱ │
                  [0]──│─────[1]  │
                   │   │      │   │
                   │  [6]─────│──[5]
                   │ ╱        │ ╱
                  [3]────────[2]
                      FRONT
```

The seven characters of `P` describe positions `1, 2, 3, 4, 5, 6, 7` in that
order; the anchor at position `0` is left out.

#### Step 4: Put the cubies into those positions

Compare the position map on the left with the filled cube on the right. Read
`P = 2134567` from left to right to fill the positions. The arrows below the
figure identify the two positions that change.

```diagram
 POSITION MAP                             AFTER P = 2134567
 (fixed seats)                            (cubies now in seats)

     [7]────────[4]                           [7]────────[4]
    ╱ │        ╱ │                           ╱ │        ╱ │
 [0]──│─────[1]  │                        [0]──│─────[2]  │
  │   │      │   │                         │   │      │   │
  │  [6]─────│──[5]                        │  [6]─────│──[5]
  │ ╱        │ ╱                           │ ╱        │ ╱
 [3]────────[2]                           [3]────────[1]
     FRONT                                    FRONT

 position:     1 2 3 4 5 6 7
 P says:       2 1 3 4 5 6 7
               │ │ └───────── cubies 3–7 stay in their matching seats
               │ └─────────── put cubie 1 in position 2: [2] becomes [1]
               └───────────── put cubie 2 in position 1: [1] becomes [2]
```

So the first two digits, `21`, exchange the two corners on the front-right
edge. The remaining digits, `34567`, leave the other five movable corners
where they were. `P` must contain every digit from `1` through `7` exactly
once; otherwise a cubie would be missing or duplicated.

The seven seats named by `P` are:

| Position | Corner of the cube |
| :---: | :--- |
| 1 | front, upper, right |
| 2 | front, down, right |
| 3 | front, down, left |
| 4 | back, upper, right |
| 5 | back, down, right |
| 6 | back, down, left |
| 7 | back, upper, left |

The second group, `O = 1111111`, describes the twist of the cubie in each of
those same seven positions:

| Digit | Meaning |
| :---: | :--- |
| 1 | not twisted |
| 2 | twisted by +120° |
| 3 | twisted by −120° |

Here every orientation digit is `1`, so the two corners change places without
being twisted. For a valid cube, convert orientation digits to `0`, `1`, and
`2`; their sum must be divisible by three. The solved code is
`12345671111111`. `make check` uses the exchanged-corner example above.

## Reading the solution

```sh
$ ./solver 21345671111111
R B' D2 R' B R' B' R D2 R B
```

Each token is one face turn. Apply them left to right; after the last one the
cube is solved.

| Token | Meaning |
| :---: | :--- |
| `R` | turn the `RIGHT` face 90° clockwise |
| `B` | turn the `BACK` face 90° clockwise |
| `D` | turn the `DOWN` face 90° clockwise |

Clockwise means clockwise as seen by someone looking directly at that face from
outside the cube, so you have to walk around to the back to read `B` and look up
from underneath to read `D`. Two suffixes modify a turn:

| Suffix | Meaning |
| :---: | :--- |
| none | 90° clockwise |
| `'` | 90° counterclockwise, the inverse |
| `2` | 180°, direction does not matter |

`R`, `B`, and `D` are the only faces that appear, because turning `UP`, `FRONT`,
or `LEFT` would move the anchor at position `0`. A turn counts as one move
whichever suffix it carries, which is the half-turn metric; under that metric no
position needs more than 11 moves. Solving an already-solved cube prints an
empty line.

See [`report.md`](report.md) for the original BFS baseline's model, algorithm,
diagrams, and Frama-C validation notes.
