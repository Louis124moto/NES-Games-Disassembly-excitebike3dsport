#!/bin/sh
# Build and run the differential (oracle) tests.  Usage: tests/run_oracle_tests.sh [source/race_x.c ...]
# With no arguments every test_race_*.c is linked against ALL source/race_*.c (integration mode).
# With arguments each test is linked only against the given sources; unported callees fall back to the original code.
set -e
cd "$(dirname "$0")/.."
OUT=${OUT:-/tmp/oracle_build}; mkdir -p "$OUT"
python3 tools/asm_to_prg.py ../Excitebike/bank_FF.asm "$OUT/prg.bin"
SRCS="$@"; [ -n "$SRCS" ] || SRCS=$(ls source/race_*.c)
rc=0
for t in tests/test_race_*.c; do
  b=$(basename "$t" .c)
  gcc -std=c11 -O2 -Wall -Wextra -Werror -Iinclude -Itests/oracle tests/oracle/*.c $SRCS "$t" -o "$OUT/$b"
  "$OUT/$b" "$OUT/prg.bin" || rc=1
done
exit $rc
