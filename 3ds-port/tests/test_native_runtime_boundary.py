#!/usr/bin/env python3
"""Guard the 3DS executable against accidental NES-emulator runtime reintroduction."""
from pathlib import Path
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

    def test_emulator_runtime_sources_are_not_in_game_source_tree(self):
        forbidden = (
            "nes_gen_code.c", "nes_call.c", "nes_menu_cpu.c",
            "nes_game.c", "nes_system.c", "nes_apu.c",
            "nes_audio_out.c", "menus.c", "menu_tape_file.c",
        )
        for filename in forbidden:
            with self.subTest(filename=filename):
                self.assertFalse((PORT / "source" / filename).exists())

    def test_makefile_has_no_emulator_exclusion_workaround(self):
        makefile = (PORT / "Makefile").read_text(encoding="utf-8")
        self.assertNotIn("EXCLUDED_RUNTIME_C", makefile)
        self.assertIn("CFILES := $(foreach dir,$(SOURCES)", makefile)
        self.assertIn("nativeMenuRenderStatus", (PORT / "source/native_menu.c").read_text(encoding="utf-8"))
        self.assertIn("nativeMenuRenderStatus(state)", (PORT / "source/renderer.c").read_text(encoding="utf-8"))

    def test_native_physics_is_part_of_default_source_glob(self):
        makefile = (PORT / "Makefile").read_text(encoding="utf-8")
        self.assertIn("wildcard $(dir)/*.c", makefile)
        self.assertTrue((PORT / "source/bike_physics.c").exists())


if __name__ == "__main__":
    unittest.main()
