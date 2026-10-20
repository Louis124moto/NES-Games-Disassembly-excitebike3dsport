# Excitebike 3DS

A Nintendo 3DS homebrew project focused on bringing **Excitebike** to the Nintendo 3DS.

> **Status: Work in progress.** This repository currently contains the original NES Excitebike disassembly and its assembly tools. A playable Nintendo 3DS port is not claimed yet.

## Repository contents

- `Excitebike/` — NES disassembly, assets, and assembly scripts.
- `_scripts/` — shared build helpers required by `Excitebike/assemble.sh`.
- `syntax_6502.xml` — optional 6502 syntax-highlighting definition.
- `.gitignore` — Git ignore rules.

Unrelated game disassemblies from the original multi-game repository have been removed from this project branch.

## Build the NES disassembly

Run these commands from the repository root.

**Linux / macOS**
```sh
sh Excitebike/assemble.sh
```

**Windows**

Run `Excitebike/assemble.bat`.

These scripts assemble the NES version. They do **not** build a Nintendo 3DS application. A native 3DS port still requires a 3DS-specific implementation and build pipeline.

## Upstream source and credits

The disassembly originates from [NES Games Disassembly](https://github.com/cyneprepou4uk/NES-Games-Disassembly). Please preserve relevant upstream credits and follow the original project's applicable terms when reusing or redistributing its work.

## Contributing

Contributions should focus on the Excitebike 3DS project. Please include build instructions and clearly state which hardware or emulator versions have been tested.
