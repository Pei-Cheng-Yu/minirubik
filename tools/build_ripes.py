"""Combine the solver and tables for Ripes's built-in assembler."""

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def prepare(text, render):
    lines = []
    inside = False
    for line in text.splitlines():
        directive = line.split("#", 1)[0].strip()
        if directive == ".if RENDER":
            if inside:
                raise ValueError("Nested renderer blocks are not supported")
            inside = True
            continue
        if directive == ".endif":
            if not inside:
                raise ValueError("Unmatched .endif")
            inside = False
            continue
        if directive.startswith((".if", ".else", ".elseif")):
            raise ValueError(f"Unsupported conditional: {directive}")
        if inside and not render:
            continue
        if directive.startswith(".equ RENDER,"):
            line = f".equ RENDER, {int(render)}"
        elif directive == ".section .rodata":
            line = ".data"
        elif re.fullmatch(r"\.balign\s+\d+", directive):
            line = f".align {int(directive.split()[1])}"
        elif re.fullmatch(r"\.align\s+\d+", directive):
            # GNU RISC-V uses a power of two; Ripes uses a byte boundary.
            line = f".align {1 << int(directive.split()[1])}"
        lines.append(line)
    if inside:
        raise ValueError("Unclosed .if RENDER")
    return "\n".join(lines) + "\n"


def build_source(render):
    names = ("solver.s", "moves_data.s", "separate_pdb_data.s")
    return "\n".join(
        prepare((ROOT / name).read_text(encoding="utf-8"), render)
        for name in names
    )


def open_ripes(executable, source):
    program = shutil.which(executable)
    if program is None:
        raise ValueError("Ripes not found; pass --ripes with its executable path")
    source_path = str(source.resolve())
    if sys.platform.startswith("linux") and program.lower().endswith(".exe"):
        source_path = subprocess.check_output(
            ["wslpath", "-w", source_path], text=True
        ).strip()
    subprocess.Popen([program, "--mode", "gui", "--src", source_path, "-t", "asm"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--render", action="store_true")
    parser.add_argument("-o", "--output", type=Path, required=True)
    parser.add_argument("--open", action="store_true", help="open the source in Ripes GUI")
    parser.add_argument("--ripes", default=os.environ.get("RIPES", "Ripes"))
    args = parser.parse_args()
    try:
        source = build_source(args.render)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(source, encoding="utf-8", newline="\n")
        print(f"Wrote {args.output}", flush=True)
        if args.open:
            open_ripes(args.ripes, args.output)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"error: {error}\n")


if __name__ == "__main__":
    main()
