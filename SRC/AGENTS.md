# AGENTS.md

Open-source Playdate Lua runtime (C + SDL2) that runs compiled `.pdx` Playdate
games natively on Linux/aarch64 (PortMaster handhelds). Not a git repo.

## Repository layout

- `SRC/` — the actual source tree. All development happens here. See
  `SRC/BUILD.md` and `SRC/README.md` for full details.
  - `SRC/src/` — the runtime, ~10k lines of C (`pd_*.c`). Biggest files:
    `pd_graphics.c` (3k), `pd_sprite.c` (1.5k), `pd_geometry.c`, `pd_main.c`.
  - `SRC/vendor/lua/` — vendored Lua 5.4.3, patched (`lundump.c/h`,
    `lobject.h`, `lopcodes.h`, `lvm.c`) to load Playdate's legacy
    5.4.0-beta bytecode. Do not upgrade or "clean up" these files.
  - `SRC/corelibs/` — Panic's CoreLibs Lua sources (runtime data, loaded at
    game start) plus `__stub.lua` (full API listing, source for stub
    generation).


## Build & run

```bash
cd SRC
make                 # native build -> ./playdate_runtime
make clean
```

Prereqs: `gcc make libsdl2-dev libsdl2-mixer-dev zlib1g-dev pkg-config`.

Run: `./playdate_runtime path/to/game.pdx` or point it at a directory
containing `roms/*.pdx` to get the ROM selection menu. Flags: `--scale N`,
`--integer-scale`, `--fullscreen`. ROMs at top level are zipped; extract
first (`unzip roms/Foo.pdx.zip -d /tmp/foo.pdx`).

aarch64 release builds are done inside an **arm64 Ubuntu 20.04 docker
container** (old glibc for device compat), not a cross toolchain — exact
command in `SRC/BUILD.md`. The Makefile's `CROSS=aarch64` path expects
`aarch64-linux-gnu-gcc` instead.

No test suite. Verify changes by building and running a game from `roms/`
(the debug env vars below make this scriptable).

## Critical gotchas

- **`LUA_32BITS` must always be defined** (Makefile does it). Without it,
  Playdate bytecode constants corrupt silently.
- The Makefile lists all `src/*.h` as prerequisites of every object, so
  header edits trigger full rebuilds. If you see inexplicable memory
  corruption after a header change, `make clean && make`.
- `src/pd_apistubs.h` is **generated** from `corelibs/__stub.lua` — don't
  edit it by hand. Regeneration script (inline Python) is in `SRC/BUILD.md`.
- `pd_runtime.h` has an `SDL_GetTicks64` compat shim for SDL2 < 2.0.18; keep
  it when touching SDL version-dependent code.
- CoreLibs are Panic's code shipped as-is; avoid modifying them except
  `__stub.lua`.

## Debug env vars (headless testing / automation)

- `PD_TRACE=1` — draw/sprite tracing
- `PD_DUMP_FB=<frame>` — write framebuffer to `/tmp/pd_fb.pbm` at that frame
- `PD_AUTO_A=<n>` — auto-press A after n frames
- `PD_MENU_PICK=<i>` / `PD_MENU_DUMP=1` — ROM menu automation
- `PD_TILT=x,y` — fake accelerometer values (kicks in after ~400 reads)

## Architecture & conventions

- Single global runtime state: `PDRuntime g_pd` (defined in `pd_main.c`,
  declared in `pd_runtime.h`). Screen is a 1-bit 400×240 framebuffer.
- Each `pd_<area>.c` file implements one API area and exposes a
  `pd_<area>_register(lua_State *L)` that installs C functions into the
  `playdate.*` Lua tables; all are called from `register_all()` in
  `pd_main.c`. Follow this pattern when adding API surface.
- Startup flow: parse pdxinfo/PDZ (`pd_pdx.c`) → create Lua state, register
  bindings + generated stubs (`pd_apistub.c`) → load CoreLibs (falls back to
  `./corelibs` when the game doesn't bundle them) → run `main.luac` → main
  loop in `pd_main.c` calls `playdate.update` per frame.
- Anything not implemented gets a no-op stub via `pd_install_api_stubs()`
  so games don't crash on missing API — check there before assuming a
  feature exists.
- Naming: `pd_` prefix for runtime functions, `LCD*`/`PD*` typedefs
  mirroring official Playdate C API names, `k`-prefixed enum constants
  (e.g. `kColorBlack`, `kBitmapUnflipped`).
- Not implemented (don't chase phantom bugs there): C-API games
  (`pdex.bin`), `playdate.sound.synth` (stubbed), `.pdv` video, networking,
  encrypted Catalog games.
