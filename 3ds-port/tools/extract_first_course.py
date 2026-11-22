#!/usr/bin/env python3
"""Extract the first Excitebike course's six-band CPU track geometry to editable JSON.

The extractor compiles and runs the already differential-tested native F4FF
course-stream decoder. It first verifies that the embedded stream bytes are
identical to bank_FF.asm. The JSON preserves raw tile values and source offsets;
--emit-c can turn an edited JSON file into a small C data header.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PORT = ROOT / "3ds-port"
DEFAULT_OUTPUT = PORT / "assets" / "course1_geometry.json"


def embedded_stream_bytes() -> tuple[int, int, bytes]:
    header = (PORT / "include" / "race_course_stream_data.h").read_text(encoding="utf-8")
    base_match = re.search(r"COURSE_STREAM_ROM_BASE\s+(0x[0-9A-Fa-f]+)", header)
    size_match = re.search(r"COURSE_STREAM_ROM_SIZE\s+(\d+)", header)
    if not base_match or not size_match:
        raise RuntimeError("cannot read course-stream base/size from its header")
    base, size = int(base_match.group(1), 16), int(size_match.group(1))

    source = (PORT / "source" / "race_course_stream_data.c").read_text(encoding="utf-8")
    start = source.find("kCourseStreamRom")
    opening = source.find("{", start)
    closing = source.find("};", opening)
    if start < 0 or opening < 0 or closing < 0:
        raise RuntimeError("cannot find kCourseStreamRom initializer")
    values = bytes(int(value, 16) for value in
                   re.findall(r"0x([0-9A-Fa-f]{2})u?", source[opening:closing]))
    if len(values) != size:
        raise RuntimeError(
            f"embedded stream has {len(values)} bytes; header declares {size}")
    return base, size, values


def verify_against_asm(temp_dir: Path) -> tuple[int, int]:
    base, size, embedded = embedded_stream_bytes()
    prg_path = temp_dir / "bank_FF.prg"
    subprocess.run(
        [sys.executable, str(PORT / "tools" / "asm_to_prg.py"),
         str(ROOT / "Excitebike" / "bank_FF.asm"), str(prg_path)],
        check=True,
    )
    prg = prg_path.read_bytes()
    offset = base - 0xC000
    original = prg[offset:offset + size]
    if original != embedded:
        mismatch = next((i for i, (a, b) in enumerate(zip(original, embedded))
                         if a != b), min(len(original), len(embedded)))
        raise RuntimeError(
            f"embedded course stream differs from ASM at \${base + mismatch:04X}")
    return base, size


def compile_and_extract(max_columns: int, temp_dir: Path) -> dict:
    executable = temp_dir / "extract_first_course"
    command = [
        "gcc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", str(PORT / "include"),
        str(PORT / "source" / "race_course_stream.c"),
        str(PORT / "source" / "race_course_stream_data.c"),
        str(PORT / "tools" / "extract_first_course.c"),
        "-o", str(executable),
    ]
    subprocess.run(command, check=True)
    result = subprocess.run(
        [str(executable), str(max_columns)],
        check=False, text=True, capture_output=True,
    )
    if result.returncode not in (0, 3):
        raise RuntimeError(result.stderr or f"geometry decoder exited {result.returncode}")
    data = json.loads(result.stdout)
    if data["summary"]["truncated"]:
        raise RuntimeError(
            f"geometry extraction reached --max-columns={max_columns}; "
            "increase the limit rather than accepting a partial course")
    if not data["summary"]["complete"]:
        raise RuntimeError("course stream did not reach a stable end/cycle")
    if not data["columns"]:
        raise RuntimeError("course stream produced no geometry columns")
    return data


def emit_c_header(data: dict, destination: Path) -> None:
    columns = data["columns"]
    if not columns:
        raise RuntimeError("cannot emit C data: no decoded columns")
    lines = [
        "/* Generated from editable course1_geometry.json. Do not edit by hand. */",
        "#ifndef EXCITEBIKE_COURSE1_GEOMETRY_H",
        "#define EXCITEBIKE_COURSE1_GEOMETRY_H",
        "#include <stdint.h>",
        f"#define EXCITEBIKE_COURSE1_GEOMETRY_COLUMNS {len(columns)}u",
        "/* Six source pages $0400-$057F, indexed by vertical band. */",
        "static const uint8_t excitebike_course1_geometry[EXCITEBIKE_COURSE1_GEOMETRY_COLUMNS][6] = {",
    ]
    for column in columns:
        values = ", ".join(value.replace("$", "0x") + "u"
                           for value in column["values_by_band"])
        lines.append(f"    {{ {values} }},")
    lines.extend([
        "};",
        "static const uint8_t excitebike_course1_geometry_course_class[EXCITEBIKE_COURSE1_GEOMETRY_COLUMNS] = {",
        "    " + ", ".join(value["course_class"].replace("$", "0x") + "u"
                            for value in columns),
        "};",
        "static const uint8_t excitebike_course1_geometry_ring_column[EXCITEBIKE_COURSE1_GEOMETRY_COLUMNS] = {",
        "    " + ", ".join(value["ring_column"].replace("$", "0x") + "u"
                            for value in columns),
        "};",
        "#endif",
        "",
    ])
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT,
                        help="JSON output path (default: 3ds-port/assets/course1_geometry.json)")
    parser.add_argument("--max-columns", type=int, default=4096,
                        help="hard safety limit; incomplete extraction is rejected")
    parser.add_argument("--emit-c", type=Path,
                        help="also emit a C header from the JSON geometry")
    parser.add_argument("--check", action="store_true",
                        help="fail unless the existing JSON exactly matches regenerated output")
    args = parser.parse_args()
    if args.max_columns < 1 or args.max_columns > 4096:
        parser.error("--max-columns must be between 1 and 4096")

    try:
        with tempfile.TemporaryDirectory(prefix="excitebike-course-") as temporary:
            temp_dir = Path(temporary)
            base, size = verify_against_asm(temp_dir)
            data = compile_and_extract(args.max_columns, temp_dir)
        data["source"]["embedded_stream_base"] = f"\${base:04X}"
        data["source"]["embedded_stream_size"] = size
        rendered = json.dumps(data, indent=2) + "\n"

        if str(args.output) == "-":
            sys.stdout.write(rendered)
        elif args.check:
            if not args.output.exists() or args.output.read_text(encoding="utf-8") != rendered:
                print(f"stale or missing generated geometry: {args.output}", file=sys.stderr)
                return 1
            print(f"verified {len(data['columns'])} columns: {args.output}")
        else:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(rendered, encoding="utf-8")
            print(f"extracted {len(data['columns'])} editable columns to {args.output}")

        if args.emit_c:
            emit_c_header(data, args.emit_c)
            print(f"wrote C geometry header: {args.emit_c}")
        return 0
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"extract_first_course: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
