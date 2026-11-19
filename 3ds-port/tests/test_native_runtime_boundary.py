#!/usr/bin/env python3
"""Guard the final 3DS executable against accidental emulator-runtime reintroduction."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
PORT = ROOT / "3ds-port"


class NativeRuntimeBoundaryTests(unittest.TestCase):
    def test_live_game_loop_does_not_boot_nes_runtime(self):
        source = (PORT / "source/game.c").read_text(encoding="utf-8")
        forbidden = (
            "nesGameInit", "nesGameBoot", "nesGameFrame",
            "nesSystemInit", "nesSystemRunFrame",
            "nesMenuCpuInit", "nesMenuCpuRun",
            "nesAudioOutInit", "nesAudioOutPush",
        )
        for symbol in forbidden:
            with self.subTest(symbol=symbol):
                self.assertNotIn(symbol, source)

    def test_emulator_runtime_objects_are_excluded_from_3ds_build(self):
        makefile = (PORT / "Makefile").read_text(encoding="utf-8")
        match = re.search(r"EXCLUDED_RUNTIME_C\s*:=([\s\\\S]*?)\nCFILES\s*:=", makefile)
        self.assertIsNotNone(match, "Makefile must declare the runtime exclusion list")
        excluded = set(re.findall(r"[A-Za-z0-9_]+\.c", match.group(1)))
        required = {
            "menus.c", "nes_gen_code.c", "nes_call.c", "nes_machine.c",
            "nes_menu_cpu.c", "nes_game.c", "nes_system.c",
            "nes_apu.c", "nes_audio_out.c",
        }
        self.assertTrue(required.issubset(excluded), f"missing exclusions: {sorted(required - excluded)}")

    def test_native_menu_boundary_is_compiled(self):
        makefile = (PORT / "Makefile").read_text(encoding="utf-8")
        source = (PORT / "source/native_menu.c").read_text(encoding="utf-8")
        renderer = (PORT / "source/renderer.c").read_text(encoding="utf-8")
        self.assertNotIn("native_menu.c", excluded)
        self.assertIn("nativeMenuRenderStatus", source)
        self.assertIn("nativeMenuRenderStatus(state)", renderer)


if __name__ == "__main__":
    unittest.main()
