#!/usr/bin/env python3
"""Rebuild the 16 KiB PRG image ($C000-$FFFF) from the hex dump comments of Excitebike/bank_FF.asm.

Lines look like `C - - - - - 0x000561 00:C551: A9 12     LDA #con` or
`- D 2 - - - 0x000014 00:C004: 9A C9     .word ofs_000...`.
Usage: asm_to_prg.py bank_FF.asm out.bin   (fails if any byte of $C000-$FFFF is left uncovered)
"""
import re, sys
pat = re.compile(r'^[-A-Z] [-A-Z0-9 ]{9,12}0x[0-9A-F]{6} 00:([0-9A-F]{4}): ((?:[0-9A-F]{2} ?)+)')
src, dst = sys.argv[1], sys.argv[2]
prg = bytearray(0x4000)
seen = bytearray(0x4000)
for line in open(src, encoding='utf-8', errors='replace'):
    m = pat.match(line)
    if not m:
        continue
    addr = int(m.group(1), 16)
    data = bytes.fromhex(m.group(2).replace(' ', ''))
    d = re.search(r'\.byte\s+([^;]*)', line)
    if d:
        items = [x.strip() for x in d.group(1).split(',') if x.strip()]
        if len(items) > 1:
            vals = []
            for it in items:
                if re.fullmatch(r'\$[0-9A-Fa-f]+', it):
                    vals.append(int(it[1:], 16))
                elif re.fullmatch(r'[0-9]+', it):
                    vals.append(int(it))
                else:
                    sys.exit('non-literal multi-byte .byte at %s: %s' % (m.group(1), line.strip()))
            data = bytes(vals)
    for i, b in enumerate(data):
        a = addr + i - 0xC000
        if 0 <= a < 0x4000:
            if seen[a] and prg[a] != b:
                sys.exit('conflict at %04X' % (a + 0xC000))
            prg[a] = b
            seen[a] = 1
missing = [i + 0xC000 for i in range(0x4000) if not seen[i]]
if missing:
    print('uncovered bytes: %d (first %04X, last %04X)' % (len(missing), missing[0], missing[-1]), file=sys.stderr)
    sys.exit(1)
open(dst, 'wb').write(prg)
print('ok: 16384 bytes')
