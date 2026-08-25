# PD4RK - Runtime for PortMaster

An open-source Playdate Lua runtime that runs Playdate Lua games natively on ARM64 handhelds via PortMaster. No x86 emulation (box64) or proprietary simulator required.

## Add Port To Handheld

Download Linux SDK here <https://play.date/dev/>, extract it and move `CoreLibs` into `playdate4rk` folder.

Copy `PD4RK.sh` and `playdate4rk` folder to `ports` on your handheld.


## Adding Games

Drop unzipped game folders into `roms/`. Example: `ports/playdate/roms/My Game.pdx`

If a game comes as `.pdx.zip`, extract first. Launching the port shows a selection menu of everything in `roms` directory.

## What Kind of Games Work?

**Supported: Lua games ONLY**

Quick check is to look inside the `.pdx` folder for:

| Contents | Verdict |
|---|---|
| `main.pdz` (no `pdex.bin`) | ✅ Should run |
| `pdex.bin` **and** `main.pdz` | ⚠️ Hybrid, usually won't run |
| `pdex.bin` only | ❌ C-API game, not supported |

---

**Partially support** 

Games run, but these features are silent/stubbed:

* `playdate.sound.synth`: full synthesis engine (most games have sound and music)
* `playdate.video`: `.pdv` playback
* `playdate.network`: HTTP/TCP (scoreboards etc. silently do nothing)

## Controls

| Handheld | Playdate |
|---|---|
| D-pad | D-pad |
| A / B | Ⓐ / Ⓑ |
| Left analog stick | 🎣 Crank |



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


## Controls

| Handheld          | Playdate         |
|-------------------|------------------|
| D-pad              | D-pad            |
| A                 | A                |
| B                 | B                |
| Left analog stick | Crank            |
| Select + Start    | Exit             |



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
