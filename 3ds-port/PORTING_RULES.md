# Rules for source-translating ASM into C (Stage 3)

Goal: faithful port, verified by DIFFERENTIAL TESTING against the original code (PRG rebuilt from the ASM by `tools/asm_to_prg.py`,
executed by the validated 6502 core in `tests/oracle/`).

1. One C function per ASM region: `void nesR_XXXX(NesMachine *m)` (XXXX = region start address, prototypes in `include/nes_race_decl.h`,
   which is GENERATED - do not edit). Put the code in `source/race_<package>.c` (+ optional `include/race_<package>.h`), test in
   `tests/test_race_<package>.c`.
2. Translate instruction by instruction. Registers live in `m->a/x/y`, flags in `m->c/z/n/v`. Use the helpers in `include/nes_machine.h`
   (`nesLoad`, `nesAdc`, `nesSbc`, `nesCmp`, `nesAsl/Lsr/Rol/Ror`, `nesSetNZ`) so flags are exact; branches test `m->z/n/c/v` as the 6502 would.
   Memory = `m->ram[addr & 0x7FF]` (zero page, OAM at $0200, etc.), ROM tables = `nesPrgRead(m, addr)` (never retype tables), hardware
   registers = `nesIoWrite/nesIoRead`. Zero-page indexed addressing wraps at 8 bits (`(uint8_t)(zp + x)`), absolute,X does not wrap.
3. Fall-through into the next label = call the next `nesR_XXXX` at the end. `JSR x` = call `nesR_x(m)`; `JMP x` = call and return (tail call).
   `RTS` = return. Calls to regions owned by another package resolve to the weak oracle fallback in tests (they run the ORIGINAL code),
   so every package is testable on its own; in the integrated build the real port is linked.
4. Do NOT add game logic that is not in the ASM; do NOT "clean up" 6502 quirks (page wrap, flag leakage, X=$FF index wraps) - keep them.
   Original bugs flagged `bzk bug?` in the comments are reproduced.
5. Every region needs a differential test via `oracleDiffRoutine(&o, 0xADDR, nesR_ADDR, name, iters, prep, cmpRegs, &skipped)`:
   random RAM/regs on both sides, results (RAM except stack page and documented ignores, registers the routine really outputs, io-write
   log) must be identical. Use `prep` only to constrain inputs to states the game can reach (e.g. indexes in range, divisors != 0) and
   comment why. Routines with loops that depend on hardware ($2002 polling) need `prep` that sets `cpu.io[...]`
   (note: reads of io registers return `cpu.io[addr]` on both sides).
   A test that skips (step limit) all iterations fails. Do mutation checks: change one operation in your port and confirm the test fails.
6. Sound-engine calls: port the caller faithfully; if the callee is in the sound engine it may stay a weak fallback in tests and a stub
   in the 3DS build (declare it in `race_sound_stub.c` owned by P5) - but note it.
7. Commit per region group on your branch; message ends with the standard Co-Authored-By / Claude-Session lines. Never touch files outside your own
   package (`source/race_<pkg>.c`, `include/race_<pkg>.h`, `tests/test_race_<pkg>.c`) except to report problems; generated files are rebuilt by tools.
8. Test command in your worktree: `tests/run_oracle_tests.sh source/race_<pkg>.c` (from `3ds-port/`).
