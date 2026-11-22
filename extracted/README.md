# Extracted course data

This directory contains source-derived, editable level data. It does not contain a ROM dump.

## First course

- `course1_geometry.json` — **1,335 ordered columns**, each carrying six 64-byte vertical-band tile IDs, PPU/ring coordinates and source provenance.
- The source stream starts at `$ED46`; the next stream pointer is `$EE59`, so stream 0 occupies 275 bytes. Its first byte sets the repeat counter to `$02`.
- The JSON preserves raw tile IDs, course/ring coordinates, PPU coordinates, stream offsets, template IDs and source addresses.
- Extraction stops at the original `$F638` path when template 9 reduces `$0057` to zero and sets `$03BC`. This is the source-defined two-pass completion signal; it is not a guessed column limit.
- `complete: true` means the decoder reached a stable terminal stream state; partial/truncated output is rejected by the reader.

Regenerate from the original disassembly and the native C stream decoder:

```sh
python3 3ds-port/tools/extract_first_course.py
```

Check that the committed file is reproducible:

```sh
python3 3ds-port/tools/extract_first_course.py --check
```

Inspect the level as raw tile IDs, six bands from top to bottom:

```sh
python3 3ds-port/tools/read_course.py
python3 3ds-port/tools/read_course.py --start 64 --count 64
python3 3ds-port/tools/read_course.py --format glyph
python3 3ds-port/tools/read_course.py --column 0
python3 3ds-port/tools/read_course.py --csv /tmp/course1.csv
```

The glyph view is a diagnostic shorthand, not a reconstruction of NES graphics. Use the hexadecimal view when exact tile IDs matter. The six bands are the original CPU-side `$0400-$057F` pages, not a decoded NES pixel image.

Edit the JSON to experiment with course geometry. Re-run the reader to inspect changes. Re-running the extractor overwrites the generated JSON with source-derived data. Runtime gameplay still consumes compiled native course data; editing this JSON alone does not silently alter the 3DS executable. Generate a C header explicitly with `--emit-c PATH` when integrating edited geometry into the native runtime.
