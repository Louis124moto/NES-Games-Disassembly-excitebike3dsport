#!/usr/bin/env python3
"""Guard the 3DS executable against accidental NES-emulator runtime reintroduction."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PORT = ROOT / "3ds-port"


class NativeRuntimeBoundaryTests(unittest.TestCase):
    def test_live_game_loop_does_not_boot_nes_runtime(self):
        source = (PORT / "source/app/game.c").read_text(encoding="utf-8")
        forbidden = (
            "nesGameInit", "nesGameBoot", "nesGameFrame",
            "nesSystemInit", "nesSystemRunFrame",
            "nesMenuCpuInit", "nesMenuCpuRun",
            "nesAudioOutInit", "nesAudioOutPush",
        )
        for symbol in forbidden:
            with self.subTest(symbol=symbol):
                self.assertNotIn(symbol, source)

    RUNTIME_MODULES = ("app", "frontend", "engine", "graphics", "audio", "platform")
    LIBCTRU_FREE = ("frontend", "engine", "assets")   # may not use libctru/citro3d
    LIBCTRU_FREE_FILES = ("graphics/native_scene.c", "audio/sound_driver.c",
                          "audio/sound_synth.c", "audio/sound_data.c")

    def _sources(self, modules):
        for module in modules:
            for path in sorted((PORT / "source" / module).glob("*.c")):
                yield module, path

    def test_runtime_has_no_ppu_or_software_framebuffer_leftovers(self):
        forbidden = ("nes_ppu_", "nesPpu", "gfxGetFramebuffer",
                     "game_state.h", "collision.h", "nes_active_stream",
                     "nes_race_setup", "native_menu_renderer")
        for module, path in self._sources(self.RUNTIME_MODULES):
            text = path.read_text(encoding="utf-8")
            for symbol in forbidden:
                with self.subTest(file=f"{module}/{path.name}", symbol=symbol):
                    self.assertNotIn(symbol, text)

    def test_source_root_holds_only_module_directories(self):
        entries = sorted(p.name for p in (PORT / "source").iterdir())
        self.assertEqual(entries, ["app", "assets", "audio", "engine", "frontend",
                                   "graphics", "platform"])

    def test_headers_live_in_include(self):
        for module in (*self.RUNTIME_MODULES, "assets"):
            for path in (PORT / "source" / module).glob("*.h"):
                with self.subTest(header=str(path)):
                    self.fail("public headers belong in include/")

    def test_deterministic_modules_do_not_use_the_platform(self):
        banned = ("<3ds.h>", "<citro3d.h>", "<citro2d.h>", "<ctru")
        files = [p for m, p in self._sources(self.LIBCTRU_FREE)]
        files += [PORT / "source" / f for f in self.LIBCTRU_FREE_FILES]
        for path in files:
            text = path.read_text(encoding="utf-8")
            for token in banned:
                with self.subTest(file=str(path.relative_to(PORT)), token=token):
                    self.assertNotIn(token, text)

    def test_engine_and_platform_have_no_upward_dependencies(self):
        owners = {}
        for module, path in self._sources((*self.RUNTIME_MODULES, "assets")):
            owners[path.stem] = module
        allowed = {
            "engine": {"engine"},
            "platform": {"platform"},
            "frontend": {"frontend", "engine"},
            "graphics": {"graphics", "frontend", "engine", "assets"},
            "audio": {"audio"},
        }
        import re
        for module, path in self._sources(allowed):
            for inc in re.findall(r'#include "([^"]+)\.h"', path.read_text(encoding="utf-8")):
                owner = owners.get(Path(inc).name)
                if owner is None:
                    continue
                with self.subTest(file=f"{module}/{path.name}", include=inc):
                    self.assertIn(owner, allowed[module])

    def test_renderer_uses_citro3d(self):
        text = (PORT / "source/graphics/gfx_c3d.c").read_text(encoding="utf-8")
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
                self.assertEqual(list((PORT / "source").rglob(filename)), [])

    def test_makefile_has_no_emulator_exclusion_workaround(self):
        makefile = (PORT / "Makefile").read_text(encoding="utf-8")
        self.assertNotIn("EXCLUDED_RUNTIME_C", makefile)
        self.assertIn("CFILES := $(foreach dir,$(SOURCES)", makefile)
        menu = (PORT / "source/frontend/native_menu.c").read_text(encoding="utf-8")
        self.assertIn("nativeMenuUpdate", menu)
        self.assertNotIn("nativeMenuRenderStatus", menu)
        self.assertNotIn("nativeMenuRenderStatus", (PORT / "source/graphics/renderer.c").read_text(encoding="utf-8"))

    def test_native_physics_is_part_of_default_source_glob(self):
        makefile = (PORT / "Makefile").read_text(encoding="utf-8")
        self.assertIn("wildcard $(dir)/*.c", makefile)
        self.assertTrue((PORT / "source/engine/bike_physics.c").exists())

    def test_linker_map_uses_resolved_output_path(self):
        makefile = (PORT / "Makefile").read_text(encoding="utf-8")
        # OUTPUT is assigned below LDFLAGS. A simply-expanded := would freeze
        # the automatic $* as empty and emit a useless ".map" in the build cwd.
        self.assertIn("LDFLAGS = -specs=3dsx.specs", makefile)
        self.assertIn("-Wl,-Map,$(OUTPUT).map", makefile)
        self.assertNotIn("$(notdir $*.map)", makefile)


if __name__ == "__main__":
    unittest.main()
