# Playdate Runtime (PortMaster Port)

Runs **Playdate** games natively on ARM64 handhelds. No simulator, no x86 emulation. An open-source reimplementation of the `playdate.*` Lua API on top of SDL2, rendering at the Playdate's 400×240 1-bit screen.

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
