"""Compare exported and linked tables byte-for-byte with the current C solver."""
import argparse
from pathlib import Path
import re
import shlex
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = r'''
#define main solver_cli_main
#include "solver.c"
#undef main
int main(void) {
    build_moves();
    fwrite(permutation, 1, sizeof permutation, stdout);
    fwrite(orientation, 1, sizeof orientation, stdout);
    fwrite(permutation_distances, 1, sizeof permutation_distances, stdout);
    fwrite(orientation_distances, 1, sizeof orientation_distances, stdout);
    return fflush(stdout) != 0 || ferror(stdout);
}
'''

def read_tables(path):
    tables = {}
    current = None
    for line in path.read_text().splitlines():
        line = line.split('#', 1)[0].strip()
        if line.endswith(':'):
            current = line[:-1]
            assert current not in tables, current
            tables[current] = bytearray()
        match = re.fullmatch(r'\.(byte|half)\s+([0-9]+)', line)
        if match:
            assert current is not None
            tables[current].extend(struct.pack('<B' if match[1] == 'byte' else '<H',
                                               int(match[2])))
    return tables

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='cc')
    parser.add_argument('--cross', default='riscv64-unknown-elf-')
    parser.add_argument('--dir', type=Path, default=ROOT / 'build/rv32')
    args = parser.parse_args()
    directory = args.dir.resolve()
    reference = directory / 'table_reference'
    subprocess.run(shlex.split(args.cc) + ['-O2', '-std=c99', '-I', str(ROOT),
                   '-x', 'c', '-', '-o', str(reference)],
                   input=REFERENCE.encode(), check=True)
    expected = subprocess.check_output([str(reference)])
    tables = read_tables(ROOT / 'moves_data.s')
    distances = read_tables(ROOT / 'separate_pdb_data.s')
    assert not tables.keys() & distances.keys()
    tables.update(distances)
    layout = [('permutation_moves', 30240), ('orientation_moves', 4374),
              ('permutation_distances', 5040), ('orientation_distances', 729)]
    assert set(tables) == {name for name, _ in layout}
    offset = 0
    for name, size in layout:
        assert len(tables[name]) == size, name
        assert tables[name] == expected[offset:offset + size], name
        offset += size
    assert len(expected) == offset == 40383
    image = directory / 'table_rodata.bin'
    subprocess.run([args.cross + 'objcopy', '-O', 'binary', '-j', '.rodata',
                    str(directory / 'solver.elf'), str(image)], check=True)
    assert image.read_bytes() == expected, 'Linked table bytes differ'
    symbols = subprocess.check_output([args.cross + 'nm', '-g', '--defined-only',
                                      str(directory / 'solver.elf')], text=True)
    addresses = {parts[2]: int(parts[0], 16) for line in symbols.splitlines()
                 if len(parts := line.split()) == 3}
    offset = addresses['permutation_moves']
    assert offset % 2 == 0
    for name, size in layout:
        assert addresses[name] == offset, name
        offset += size
    print('PASS: all 17307 move entries and 5769 distance entries match C')
    print('PASS: linked bytes, global labels and move-table alignment; 40383 bytes')

if __name__ == '__main__':
    main()
