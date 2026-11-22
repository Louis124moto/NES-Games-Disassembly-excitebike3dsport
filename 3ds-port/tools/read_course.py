#!/usr/bin/env python3
"""Read and inspect editable six-band Excitebike course geometry JSON."""
from __future__ import annotations

import argparse
import csv
import json
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_INPUT = ROOT / "extracted" / "course1_geometry.json"
BANDS = 6
GLYPHS = {
    0x3B: "#", 0x3C: "=", 0x3D: "-", 0xE4: ".", 0xE6: "^", 0xFC: " ",
}


def byte_value(value: Any, where: str) -> int:
    if not isinstance(value, str):
        raise ValueError(f"{where}: expected a hexadecimal byte string")
    token = value.strip()
    try:
        number = int(token[1:], 16) if token.startswith("$") else int(token, 0)
    except ValueError as exc:
        raise ValueError(f"{where}: invalid byte {value!r}") from exc
    if not 0 <= number <= 0xFF:
        raise ValueError(f"{where}: byte out of range: {value!r}")
    return number


def validate_course(data: dict[str, Any]) -> list[dict[str, Any]]:
    if data.get("schema") != "excitebike.first-course-geometry.v1":
        raise ValueError("unsupported or missing course geometry schema")
    summary = data.get("summary")
    columns = data.get("columns")
    if not isinstance(summary, dict) or not isinstance(columns, list):
        raise ValueError("missing summary or columns")
    if summary.get("complete") is not True or summary.get("truncated") is not False:
        raise ValueError("course file is incomplete or truncated; re-run the extractor")
    if summary.get("column_count") != len(columns) or not columns:
        raise ValueError("summary column_count does not match the non-empty columns array")
    for ordinal, column in enumerate(columns):
        if not isinstance(column, dict) or column.get("ordinal") != ordinal:
            raise ValueError(f"column {ordinal}: invalid object or ordinal")
        values = column.get("values_by_band")
        if not isinstance(values, list) or len(values) != BANDS:
            raise ValueError(f"column {ordinal}: expected exactly six vertical bands")
        for band, value in enumerate(values):
            byte_value(value, f"column {ordinal}, band {band}")
        for field in ("course_class", "ring_column", "ppu_row", "nametable"):
            if field not in column:
                raise ValueError(f"column {ordinal}: missing {field}")
            byte_value(column[field], f"column {ordinal}, {field}")
    return columns


def render_hex(columns: list[dict[str, Any]], start: int, count: int) -> str:
    stop = min(len(columns), start + count) if count else len(columns)
    if start >= len(columns):
        raise ValueError(f"start column {start} is outside the {len(columns)}-column course")
    width = len(str(stop - 1))
    lines = [f"columns {start}..{stop - 1} (hex tile IDs; band 0 is uppermost)"]
    lines.append("band | " + " ".join(f"{i:02X}" for i in range(start, stop)))
    lines.append("-----+" + "-" * (1 + 3 * (stop - start)))
    for band in range(BANDS):
        cells = [f"{byte_value(columns[i]['values_by_band'][band], 'tile'):02X}"
                 for i in range(start, stop)]
        lines.append(f"  {band}  | " + " ".join(cells))
    return "\n".join(lines)


def render_glyphs(columns: list[dict[str, Any]], start: int, count: int) -> str:
    stop = min(len(columns), start + count) if count else len(columns)
    if start >= len(columns):
        raise ValueError(f"start column {start} is outside the {len(columns)}-column course")
    lines = [f"columns {start}..{stop - 1} (diagnostic symbols, not NES artwork)"]
    for band in range(BANDS):
        cells = []
        for i in range(start, stop):
            value = byte_value(columns[i]["values_by_band"][band], "tile")
            cells.append(GLYPHS.get(value, "?"))
        lines.append(f"{band}: " + "".join(cells))
    lines.append("legend: #=$3B  =$3C  -=$3D  .= $E4  ^=$E6  ?=other tile ID")
    return "\n".join(lines)


def print_column(column: dict[str, Any]) -> None:
    print(f"column #{column['ordinal']}")
    for field in ("course_class", "ring_column", "ppu_row", "nametable",
                  "stream_offset_before", "stream_address_before",
                  "stream_offset_after", "template_id", "template_address",
                  "template_offset_next"):
        if field in column:
            print(f"  {field}: {column[field]}")
    print("  vertical bands (0=top, 5=bottom):")
    semantics = column.get("semantics_by_band", [])
    for band, value in enumerate(column["values_by_band"]):
        suffix = f"  {semantics[band]}" if len(semantics) == BANDS else ""
        print(f"    {band}: {value}{suffix}")


def write_csv(path: Path, columns: list[dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.writer(output)
        writer.writerow(["ordinal", "course_class", "ring_column", "ppu_row",
                         "nametable", *[f"band_{i}" for i in range(BANDS)]])
        for column in columns:
            writer.writerow([column["ordinal"], column["course_class"],
                             column["ring_column"], column["ppu_row"],
                             column["nametable"], *column["values_by_band"]])


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", type=Path, default=DEFAULT_INPUT,
                        help="geometry JSON (default: extracted/course1_geometry.json)")
    parser.add_argument("--start", type=int, default=0, help="first column to display")
    parser.add_argument("--count", type=int, default=64,
                        help="columns to display; 0 displays the whole course")
    parser.add_argument("--format", choices=("hex", "glyph"), default="hex",
                        help="hex tile IDs or a compact diagnostic-symbol map")
    parser.add_argument("--column", type=int, help="inspect one column's full metadata")
    parser.add_argument("--csv", type=Path, help="also export all columns to CSV")
    parser.add_argument("--validate", action="store_true",
                        help="validate the file without rendering the map")
    args = parser.parse_args()
    if args.start < 0 or args.count < 0 or (args.column is not None and args.column < 0):
        parser.error("column indices and count must be non-negative")
    try:
        data = json.loads(args.json.read_text(encoding="utf-8"))
        columns = validate_course(data)
        if args.csv:
            write_csv(args.csv, columns)
            print(f"wrote {len(columns)} columns to {args.csv}")
        if args.validate:
            print(f"valid complete course: {len(columns)} columns")
            return 0
        if args.column is not None:
            if args.column >= len(columns):
                raise ValueError(f"column {args.column} is outside the {len(columns)}-column course")
            print_column(columns[args.column])
        elif args.format == "hex":
            print(render_hex(columns, args.start, args.count))
        else:
            print(render_glyphs(columns, args.start, args.count))
        return 0
    except (OSError, json.JSONDecodeError, ValueError) as error:
        print(f"read_course: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
