# Graphics assets — faithful-port policy

Place converted 3DS-ready assets here: textures, track data, sprites, and audio. Do not commit generated build output or a copyrighted ROM dump.

## CHR inspection pipeline started

`../tools/nes_chr_to_ppm.py` decodes 2bpp NES CHR tiles from an iNES ROM with CHR-ROM or a raw CHR byte dump into a dependency-free PPM diagnostic atlas:

```sh
python3 3ds-port/tools/nes_chr_to_ppm.py Excitebike.nes /tmp/excitebike-chr.ppm
```

The grayscale atlas shows tile pixel indices only. It is **not** an authentic in-game texture: the game's palette selection, attribute tables, sprite/background separation, transparency, scrolling, and original rendering order still need to be reconstructed from `Excitebike/bank_FF.asm` and the original graphics data.

## Next asset milestones

- [ ] Identify the exact original CHR source and record its provenance/hash; do not commit a ROM dump.
- [ ] Trace palette and attribute-table writes from the original assembly.
- [ ] Build a manifest mapping source tile index and palette to a converted asset.
- [ ] Add a native 3DS texture upload path and test tile orientation, transparency, and palette indices.
- [ ] Compare screenshots against the original NES reference before marking texture fidelity verified.

The current renderer remains a diagnostic console. This pipeline does not yet add textures to gameplay.
