# Extracted course data

This directory contains source-derived, editable level data. It does not contain a ROM dump.

## First course

- `course1_geometry.json` — **1,335 ordered columns**, each carrying six vertical-band track-page bytes, the complete 18-byte `$0300-$0311` F4FF buffer, `$03A8` spawn-request state, stream counters, PPU/ring coordinates and source provenance.
- The source stream starts at `$ED46`; the next stream pointer is `$EE59`, so stream 0 occupies 275 bytes. Its first byte sets the repeat counter to `$02`.
- Schema v2 preserves raw track-page bytes, all 18 stream-buffer bytes, `$03A8` spawn requests, `$0057/$03A4/$03A5` counters, template IDs, stream offsets, course/ring coordinates and source addresses.
- Extraction stops at the original `$F638` path when template 9 reduces `$0057` to zero and sets `$03BC`. This is the source-defined two-pass completion signal; it is not a guessed column limit.
- `complete: true` means the source-defined F638 lap counter reached zero and set `$03BC`; partial/truncated output is rejected by the reader.
- `semantics_by_band` contains source-oriented annotations for a few recognized byte classes only. Treat the raw bytes as authoritative; full terrain, ramp, obstacle and collision semantics are still being traced.

Regenerate from the original disassembly and the native C stream decoder:

```sh
python3 3ds-port/tools/extract_first_course.py
```

Check that the committed file is reproducible:

```sh
python3 3ds-port/tools/extract_first_course.py --check
python3 3ds-port/tools/extract_first_course.py --emit-c /tmp/course1_geometry.h
```

Inspect the level as raw track-page bytes, six bands from top to bottom:

```sh
python3 3ds-port/tools/read_course.py
python3 3ds-port/tools/read_course.py --start 64 --count 64
python3 3ds-port/tools/read_course.py --count 0  # all 1,335 columns
python3 3ds-port/tools/read_course.py --format glyph
python3 3ds-port/tools/read_course.py --column 0
python3 3ds-port/tools/read_course.py --csv /tmp/course1.csv
```

The glyph view is a diagnostic shorthand, not a reconstruction of NES graphics. Use the hexadecimal view when exact raw byte values matter. The six bands are the original CPU-side `$0400-$057F` pages, not a decoded NES pixel image.

Edit the JSON to experiment with course geometry. Re-run the reader to inspect changes. Re-running the extractor overwrites the generated JSON with source-derived data. Runtime gameplay still consumes compiled native course data; editing this JSON alone does not silently alter the 3DS executable. `--emit-c PATH` exports the complete extracted track-page arrays, stream metadata, spawn-request flags and 18-byte F4FF buffers to a C header for native runtime integration.
