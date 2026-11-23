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

    def test_runtime_has_no_ppu_or_software_framebuffer_leftovers(self):
        runtime = ("game.c", "renderer.c", "main.c", "native_scene.c", "gfx_c3d.c")
        forbidden = ("nes_ppu_", "nesPpu", "gfxGetFramebuffer",
                     "game_state.h", "collision.h", "nes_active_stream",
                     "nes_race_setup", "native_menu_renderer")
        for filename in runtime:
            text = (PORT / "source" / filename).read_text(encoding="utf-8")
            for symbol in forbidden:
                with self.subTest(file=filename, symbol=symbol):
                    self.assertNotIn(symbol, text)

    def test_renderer_uses_citro3d(self):
        text = (PORT / "source/gfx_c3d.c").read_text(encoding="utf-8")
        self.assertIn("C3D_FrameBegin", text)
        self.assertIn("C3D_TexInit", text)

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
        menu = (PORT / "source/native_menu.c").read_text(encoding="utf-8")
        self.assertIn("nativeMenuUpdate", menu)
        self.assertNotIn("nativeMenuRenderStatus", menu)
        self.assertNotIn("nativeMenuRenderStatus", (PORT / "source/renderer.c").read_text(encoding="utf-8"))

    def test_native_physics_is_part_of_default_source_glob(self):
        makefile = (PORT / "Makefile").read_text(encoding="utf-8")
        self.assertIn("wildcard $(dir)/*.c", makefile)
        self.assertTrue((PORT / "source/bike_physics.c").exists())

    def test_linker_map_uses_resolved_output_path(self):
        makefile = (PORT / "Makefile").read_text(encoding="utf-8")
        # OUTPUT is assigned below LDFLAGS. A simply-expanded := would freeze
        # the automatic $* as empty and emit a useless ".map" in the build cwd.
        self.assertIn("LDFLAGS = -specs=3dsx.specs", makefile)
        self.assertIn("-Wl,-Map,$(OUTPUT).map", makefile)
        self.assertNotIn("$(notdir $*.map)", makefile)


if __name__ == "__main__":
    unittest.main()
