#!/usr/bin/env python3
"""Call graph of Excitebike/bank_FF.asm regions (sub_/loc_/ofs_ labels). Usage: callgraph.py bank_FF.asm ROOT_ADDR..."""
import re, sys, json
lines = open(sys.argv[1], encoding='utf-8', errors='replace').read().split('\n')
labre = re.compile(r'^((?:sub|loc|ofs|bra|vec)_\w+):')
insre = re.compile(r'^[-A-Z] [-A-Z0-9 ]{9,12}0x[0-9A-F]{6} 00:([0-9A-F]{4}): ((?:[0-9A-F]{2} ?)+)\s*(\w{3})\s*([^;]*)')
regions = []   # (name, addr, [instr...])
cur = None
pending = []
for ln in lines:
    m = labre.match(ln)
    if m:
        n = m.group(1)
        if n.startswith('bra_'): continue
        pending.append(n); continue
    m = insre.match(ln)
    if m:
        addr = int(m.group(1), 16); mn = m.group(3); op = m.group(4).strip()
        if pending:
            cur = {'names': pending, 'addr': addr, 'ins': []}; regions.append(cur); pending = []
        if cur is None: continue
        cur['ins'].append((addr, mn, op))
byname = {}
for r in regions:
    for n in r['names']: byname[n] = r
def addr_of(name):
    r = byname.get(name); return r['addr'] if r else None
# label lookup also for bra_ inside regions: map bra to its region by scanning
bra_region = {}
cur = None; pending_b = []
for ln in lines:
    m = labre.match(ln)
    if m:
        n = m.group(1)
        if n.startswith('bra_'): pending_b.append(n)
        else:
            pending_b = []
        continue
    m = insre.match(ln)
    if m:
        a = int(m.group(1), 16)
        for b in pending_b: bra_region[b] = a
        pending_b = []
region_of_addr = {}
for r in regions:
    for (a, mn, op) in r['ins']: region_of_addr[a] = r
def callees(r):
    out = set()
    last = None
    for (a, mn, op) in r['ins']:
        last = mn
        if mn in ('JSR', 'JMP'):
            t = op.split()[0] if op else ''
            if t in byname: out.add(byname[t]['addr'])
        # jump tables (.word) are not followed
    # fallthrough
    if last not in ('RTS', 'RTI', 'JMP', 'BRK') and r['ins']:
        nxt = r['ins'][-1][0] + 1
        # next region by address order
    return out
order = sorted(regions, key=lambda r: r['addr'])
fall = {}
for i, r in enumerate(order[:-1]):
    last = r['ins'][-1][1] if r['ins'] else None
    if last not in ('RTS', 'RTI', 'JMP', 'BRK'): fall[r['addr']] = order[i + 1]['addr']
byaddr = {r['addr']: r for r in regions}
def closure(root):
    seen = set(); st = [root]
    while st:
        a = st.pop()
        if a in seen or a not in byaddr: continue
        seen.add(a)
        for c in callees(byaddr[a]): st.append(c)
        if a in fall: st.append(fall[a])
    return seen
if __name__ == '__main__':
    roots = [int(x, 16) for x in sys.argv[2:]]
    res = {}
    for rt in roots:
        cl = closure(rt)
        res['%04X' % rt] = sorted(cl)
    json.dump({'closure': res, 'size': {'%04X' % a: len(byaddr[a]['ins']) for a in byaddr}, 'names': {'%04X' % a: byaddr[a]['names'][0] for a in byaddr}}, open(sys.argv[1].replace('.asm', '') + '.cg.json' if False else '/tmp/cg.json', 'w'))
    tot = set()
    for rt in roots:
        cl = closure(rt)
        n = sum(len(byaddr[a]['ins']) for a in cl)
        print('%04X' % rt, byaddr[rt]['names'][0], 'regions', len(cl), 'instr', n)
        tot |= cl
    print('TOTAL regions', len(tot), 'instr', sum(len(byaddr[a]['ins']) for a in tot))
