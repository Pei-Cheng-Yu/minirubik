# Generate valid cube test inputs

`gen_states.c` is an AI-assisted host utility. It runs BFS from the solved cube
using the existing cube move tables, then emits inputs at a chosen exact
distance. It does not count heuristic calls or execute the assembly solver.

From `HW1/minirubik` in WSL:

```sh
make states
head build/states/distance11.csv
```

This generates all 2,644 distance-11 states in rank order. To choose a different
distance after building the tool:

```sh
./build/states/gen_states 0 > build/states/distance0.csv
./build/states/gen_states 1 > build/states/distance1.csv
```

Accepted distances are 0 through 11; omitting the argument selects 11. The tool
always performs the full BFS, even for distance 0. CSV goes to stdout and the
generation summary goes to stderr. Exit status is 0 on success, 1 on failure,
and 2 for invalid command-line arguments.

Output format:

```csv
rank,input,distance
275,12345672121232,11
```

## How the states are generated

1. Enqueue the solved state (rank 0), with distance 0.
2. Dequeue a state and apply all nine moves: R/R2/R', B/B2/B', D/D2/D'.
3. On the first visit to a neighbor, record the parent's distance plus one and
   enqueue it. Each of the nine moves costs one, including a half turn.
4. After BFS, decode states at the requested distance and print their input
   strings. Check each emitted state's validity and rank/unrank round trip.

Starting from solved and following legal moves establishes reachability,
assuming the shared cube model is correct. BFS first-discovery depth gives
the shortest distance in this move metric. The utility verifies that it
reaches all 3,674,160 states. An 11-move scramble alone would not establish
shortest distance 11; moves can cancel or admit a shorter solution.

The generator includes `solver.c` with its CLI main renamed, so it can reuse
the existing static cube functions without modifying the solver. Consequently,
building it requires `separate_pdb_data.h`; make generates that dependency if missing.
However, BFS never calls the PDB heuristic or IDA*. This is not an independent
implementation of the move model.

The full distance array and BFS queue use about 17.5 MiB of host storage.
They are not linked into the RISC-V program. Generated files stay under the
ignored `build/` directory. Keep this tool's generation results distinct from
your own solver performance measurements and disclose assistance as applicable.

The earlier combined helper `tests/rv32_oracle.c` also generates a distance-11
list (`build/rv32/hard.csv`), but additionally checks heuristics and emits
assembly test fixtures. This tool is a smaller entry point for state generation.

## Count heuristic evaluations for chosen inputs

`solver.c` has an optional counter at the entry of `heuristic()`, enabled only
when `COUNT_HEURISTIC` is defined. Normal solver builds contain no counter or
increment. `tools/count_heuristic.c` enables it and includes the existing solver
in the same compilation unit so its static functions are available to the driver.

From the repository root in WSL:

```sh
make heuristic-counter
./build/states/count_heuristic 21345671111111 12345672121232 54721631111111 > build/states/heuristic_counts.csv
```

Arguments are input strings from the CSV's `input` column, not full CSV rows.
The output columns are `input,solution_length,heuristic_calls`. The driver
validates inputs, builds move tables once, resets the counter before each solve,
and verifies the returned path by replaying physical moves. Replay does not call
the heuristic. It does not independently prove optimality; use the BFS distance
file for the exact expected length.

The counter includes the root evaluation and every candidate evaluation across
all thresholds, including candidates later pruned. It counts repeated states
again if their heuristic is recalculated. It does not count skipped moves or
reuse of a cached heuristic value. These are C operation counts, not Ripes
instruction counts or native timing results. Regenerate them when the search
or heuristic changes. This diagnostic driver and counter are AI-assisted;
existing assistant-run results do not claim personal student measurements.

## Separate permutation/orientation PDBs

The C solver now reads `permutation_distances[p]` and `orientation_distances[o]`
and returns their maximum. Each table stores one byte per rank: 5,040 + 729 =
5,769 bytes. No decoding, four-cubie projection, pattern ranking or nibble
unpacking is needed during heuristic evaluation. IDA* traversal is unchanged.

`gen_pdb.c` builds both tables on the host and emits
`separate_pdb_data.h`. The move definitions live directly in `solver.c` and
`gen_pdb.c`; there is no extra move header. The generator uses factorial/base-3
decoding on the host; its queues and transition arrays
are not part of the generated header. It does not include solver.c or depend
on a previously generated table, so a clean build has no dependency cycle.

```sh
make solver heuristic-counter
make check check-pdb
```

`check-pdb` checks table completeness, exact projected distances, and H1 over
all 3,674,160 full states. It also checks optimal solution lengths and replays
all 2,644 distance-11 states plus deterministic samples at other depths.
This is not a full H3 sweep of every starting state. These tables use bytes,
so packed-entry checks do not apply to this variant.

The former four-cubie generator is preserved in Git history. `gen_pdb.c` now
generates only the separate tables; `make check-pdb` checks this version.
The solver assembly now uses the separate-PDB search. Regenerate C call
counts into a new file, such as `build/states/heuristic_counts_separate.csv`,
to preserve the earlier four-cubie measurements for comparison.


## Assembly table export

`make rv32-tables` writes `moves_data.s` and `separate_pdb_data.s` in the
repository root so they can be committed. The former exports the halfword labels
`permutation_moves` and `orientation_moves` in R, B, D row order; the latter
exports the byte labels `permutation_distances` and `orientation_distances`.
The linker includes both objects with `solver.s` when running `make rv32`.
Set `CROSS` to your GNU RISC-V tool prefix if it is not on PATH.

`make check-table-exports` checks every emitted entry against the current C
arrays, then checks the linked `.rodata` bytes, global label addresses and
halfword alignment. The four tables occupy 40,383 bytes. This checks data
export and linking, not solver execution.
`make check-pdb` separately checks the distance tables against full-state BFS.

## Ripes GUI animation

Run `make rv32-gui` to generate `build/rv32/solver_gui.s`. Open that file as
assembly source in Ripes. Keep editing `solver.s`, not the generated file.
The builder includes the renderer, combines both table files, removes the
conditional directives unsupported by the tested Ripes assembler, and converts
GNU section/alignment directives to Ripes syntax. The renderer-selection approach
is also used by [illdoCc's build script](https://github.com/illdoCc/minirubik/blob/main/tools/build_ripes.py).

Building the GUI source does not require Ripes to be installed or its path to
be configured. After cloning, run `make rv32-gui` from the repository root
(requires Make, Python 3.10 or newer, and a host C compiler for table generation),
then open `build/rv32/solver_gui.s` manually in Ripes. This source build does not
use `link.ld`; Ripes's built-in assembler resolves its labels and data layout.

To generate and open the file in one command, with Ripes on PATH:

```sh
make run-rv32-gui
```

If Ripes is not on PATH, supply its location on that machine using
`make run-rv32-gui RIPES="/path/to/Ripes"`. This is a local command-line override,
not a repository setting. From WSL, it may point to a Windows executable under
`/mnt/c/`; the builder converts the source path to Windows format.

From PowerShell, with Python on PATH, generate the source without launching Ripes:

```powershell
python tools/build_ripes.py --render -o build/rv32/solver_gui.s
```

The direct Python command uses the existing root table exports. Run
`make rv32-tables` first if their generator inputs changed.

In Ripes, instantiate LED Matrix 0 with Width 35 and Height 25, select a
32-bit ISA simulator or a processor with hazard handling, and disable ISA
extensions for the assignment. Rebuild with Ctrl+B after changing the peripheral
configuration. Pop out the LED window, reset with F3, and run with F8.
The initial cube remains visible while IDA* searches, then each solution move
is replayed and displayed. This command opens the GUI; it does not create the
peripheral or press Run for you.

For CLI measurement, continue using `make rv32` and the linked
`build/rv32/solver.elf`. That build uses GNU binutils and `link.ld`, with
`RENDER` set to 0 in `solver.s`. Coordinate replay and its final solved check run
in both builds; array decoding, drawing, and delays are GUI-only. Do not use
the GUI build's instruction count as the renderer-disabled measurement.
