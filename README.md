# Playdate Runtime for PortMaster

An open-source Playdate Lua runtime that runs Playdate Lua games natively on ARM64 handhelds via PortMaster. No x86 emulation (box64) or proprietary simulator required.

## How It Works

The runtime re-implements the Playdate Lua API on top of SDL2:

1. **Patched Lua 5.4.3** — vendored and patched to load Playdate's custom bytecode format (32-bit ints/floats, 5.4.0-beta opcodes, legacy constant tags)
2. **PDX/PDZ parser** — extracts game files from the Playdate package format
3. **`playdate.*` bindings** — C functions registered into Lua that implement the Playdate API using SDL2:
   - `playdate.graphics` — 1-bit framebuffer rendering (400×240), bitmaps, fonts, drawing primitives
   - `playdate.display` — refresh rate, inversion, scaling
   - `playdate.input` — buttons (D-pad + A/B), crank (right analog stick)
   - `playdate.file` / `playdate.datastore` — save file persistence
   - `playdate.sound` — sample/file players (stub; full synth engine TODO)
4. **CoreLibs** — Panic's standard Lua libraries (sprites, animation, timers, etc.) shipped as-is

## What Works

- Lua-only Playdate games (`.luac` bytecode in `.pdz` containers)
- Basic 2D graphics: fillRect, drawLine, drawRect, drawText, bitmaps
- Input: D-pad, A/B buttons, crank (mapped to right analog stick)
- Save data persistence via `playdate.datastore`
- CoreLibs (sprites, animation, timers, object system, etc.)

## What Doesn't Work (Yet)

- C-API Playdate games (those with `pdex.bin`) — requires ARM CPU emulation, use box64+simulator instead
- `playdate.sound.synth` — the full audio synthesis engine is stubbed
- `playdate.video` — `.pdv` video playback not implemented
- `playdate.network` — HTTP/TCP networking stubbed
- Encrypted/DRM-protected games from the Playdate Catalog

## Building

### PortMaster aarch64 Cross-Compile (Ubuntu 22.04 chroot)

```bash
# In the PortMaster build environment:
sudo apt install libsdl2-dev libsdl2-mixer-dev zlib1g-dev pkg-config

cd playdate-portmaster
make CROSS=aarch64
```

### Native Build (Linux)

```bash
sudo apt install libsdl2-dev libsdl2-mixer-dev zlib1g-dev
make
```

### macOS (for development)

```bash
brew install sdl2 sdl2_mixer
make
```

## Running

```bash
# Extract a Playdate .pdx.zip game
unzip game.pdx.zip -d game_pdx/

# Run it
./playdate_runtime game_pdx/
```

## PortMaster Installation

1. Build for aarch64: `make CROSS=aarch64`
2. Copy `playdate_runtime` to `ports/playdate/playdate_runtime.aarch64`
3. Copy `corelibs/` to `ports/playdate/corelibs/`
4. Copy `packaging/Playdate.sh` to `ports/Playdate.sh`
5. Copy `packaging/playdate.gptk` to `ports/playdate/playdate.gptk`
6. Extract your Playdate game's `.pdx.zip` contents to `ports/playdate/gamedata/`

## Controls

| Handheld          | Playdate         |
|-------------------|------------------|
| D-pad              | D-pad            |
| A                 | A                |
| B                 | B                |
| Right analog stick | Crank            |
| Select + Start    | Exit             |

## Architecture

```
game.pdx/
├── pdxinfo              ← game metadata
├── main.luac            ← compiled Lua bytecode (in .pdz)
└── assets/              ← images, fonts, audio

         │
    ┌────▼─────────────────┐
    │  playdate_runtime     │
    │  ┌──────────────────┐ │
    │  │ Lua 5.4.3         │ │  ← patched for Playdate bytecode
    │  │ (32-bit, compat)  │ │
    │  └────────┬─────────┘ │
    │           │           │
    │  ┌────────▼─────────┐ │
    │  │ playdate.* C APIs │ │  ← graphics, input, display, file
    │  │ (SDL2 bindings)   │ │
    │  └────────┬─────────┘ │
    │  ┌────────▼─────────┐ │
    │  │ SDL2 + GLES      │ │  ← native aarch64
    │  └──────────────────┘ │
    └───────────────────────┘
```

## License

- Runtime code: MIT
- Lua 5.4.3: MIT (see vendor/lua/)
- CoreLibs: Copyright Panic, Inc. (distributed as part of the Playdate SDK)

## Acknowledgments

- [Panic](https://panic.com) for the Playdate hardware and SDK
- [notpeter/playdate-lua](https://github.com/notpeter/playdate-lua) for the Lua bytecode compatibility patch
- [cranksters/playdate-reverse-engineering](https://github.com/cranksters/playdate-reverse-engineering) for file format documentation
- [TheLogicMaster/Cranked](https://github.com/TheLogicMaster/Cranked) for reference implementation
- [PortMaster](https://portmaster.games) for the handheld gaming platform
