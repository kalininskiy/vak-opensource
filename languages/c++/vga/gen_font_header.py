#!/usr/bin/env python3
#
# Generate vga_font_9x16.h from pcface graph.txt format.
# Usage: gen_font_header.py [graph.txt]
#   If no file given, reads from stdin.
#
# Source: https://github.com/susam/pcface/raw/refs/heads/main/out/oldschool-vga-9x16/graph.txt
#
import re
import sys
from pathlib import Path

BLOCK_HEADER = re.compile(r"^----- \[(.)\] \((\d+)\) -----")
ROW_LINE = re.compile(r"^0x([0-9a-fA-F]+)\s+")

NUM_GLYPHS = 256
ROWS_PER_GLYPH = 16


def parse_graph(lines):
    """Parse graph.txt lines; yield (code, [row0, row1, ...]) for each glyph."""
    code = None
    rows = []
    for line in lines:
        line = line.rstrip("\n")
        m = BLOCK_HEADER.match(line)
        if m:
            if code is not None and len(rows) == ROWS_PER_GLYPH:
                yield (code, rows)
            code = int(m.group(2))
            rows = []
            continue
        m = ROW_LINE.match(line)
        if m and code is not None:
            val = int(m.group(1), 16)
            rows.append(val & 0x1FF)  # 9 bits
    if code is not None and len(rows) == ROWS_PER_GLYPH:
        yield (code, rows)


def main():
    if len(sys.argv) >= 2:
        path = Path(sys.argv[1])
        if not path.exists():
            print(f"Error: file not found: {path}", file=sys.stderr)
            sys.exit(1)
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            lines = f.readlines()
    else:
        lines = sys.stdin.readlines()

    glyphs = [None] * NUM_GLYPHS
    for code, rows in parse_graph(lines):
        if 0 <= code < NUM_GLYPHS:
            glyphs[code] = rows

    for i in range(NUM_GLYPHS):
        if glyphs[i] is None:
            glyphs[i] = [0] * ROWS_PER_GLYPH

    out_dir = Path(__file__).resolve().parent.parent
    out_path = out_dir / "vga_font_9x16.h"

    with open(out_path, "w", encoding="utf-8") as f:
        f.write("/* VGA 9x16 font from pcface graph.txt - do not edit by hand */\n")
        f.write("#ifndef VGA_FONT_9X16_H\n")
        f.write("#define VGA_FONT_9X16_H\n\n")
        f.write("#include <cstdint>\n\n")
        f.write("constexpr unsigned VGA_FONT_GLYPHS = 256;\n")
        f.write("constexpr unsigned VGA_FONT_ROWS = 16;\n\n")
        f.write("inline const uint16_t vga_font_9x16[256][16] = {\n")
        for code in range(NUM_GLYPHS):
            rows = glyphs[code]
            row_str = ", ".join(f"0x{r:03x}u" for r in rows)
            f.write(f"  {{ {row_str} }},\n")
        f.write("};\n\n")
        f.write("#endif /* VGA_FONT_9X16_H */\n")

    print(f"Wrote {out_path}", file=sys.stderr)


if __name__ == "__main__":
    main()
