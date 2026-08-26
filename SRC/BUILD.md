# Building the Playdate runtime

Self-contained source tree for the open-source Playdate Lua runtime
(SDL2-based, runs compiled .pdx Lua games, includes the ROM selection menu).

## Contents

- `src/` — the runtime (~6k lines C). `pd_apistubs.h` is generated from
  `CoreLibs/__stub.lua` (see below).
- `vendor/lua/` — patched Lua 5.4.3. The patch (lundump.c/h, lobject.h,
  lopcodes.h, lvm.c) accepts Playdate's legacy 5.4.0-beta bytecode.
  `LUA_32BITS` must ALWAYS be defined (Makefile does this) or bytecode
  constants corrupt.
- `CoreLibs/` — Panic's CoreLibs Lua sources (runtime data; the runtime
  falls back to `./CoreLibs` when a game doesn't bundle them) plus
  `__stub.lua`, the full API listing used to generate stubs.
- `Makefile`, `CMakeLists.txt`

## Native build (Linux)

Prerequisites: `gcc make libsdl2-dev libsdl2-mixer-dev zlib1g-dev pkg-config`

```bash
make            # -> ./playdate_runtime
```

Run a game or a roms folder (menu):

```bash
./playdate_runtime path/to/game.pdx
./playdate_runtime path/to/playdate/     # dir containing roms/*.pdx
```

Flags: `--scale N`, `--integer-scale`, `--fullscreen`.

## aarch64 build for PortMaster handhelds

Build inside an arm64 Ubuntu 20.04 container (old glibc = wide device
compatibility). Needs docker with qemu binfmt (`docker run --platform
linux/arm64 alpine uname -m` should print `aarch64`):

```bash
docker run --rm --platform linux/arm64 -v "$PWD:/src" -w /src ubuntu:20.04 \
  bash -c "export DEBIAN_FRONTEND=noninteractive && \
    apt-get update -qq && \
    apt-get install -y -qq gcc make libsdl2-dev libsdl2-mixer-dev zlib1g-dev pkg-config && \
    make clean && make"
# -> playdate_runtime (ELF ARM aarch64); rename to playdate_runtime.aarch64
```

Note: Ubuntu 20.04's SDL2 lacks SDL_GetTicks64; `src/pd_runtime.h` has a
compat shim, no action needed.

## Regenerating the API stub list

If `CoreLibs/__stub.lua` changes:

```bash
python3 - <<'EOF'
import re
paths = set()
for line in open('CoreLibs/__stub.lua'):
    m = re.match(r'function\s+([\w.:]+)\s*\(', line)
    if m and not '__' in m.group(1):
        p = m.group(1)
        if p.startswith(('playdate', 'table.', 'json')):
            paths.add(p)
with open('src/pd_apistubs.h', 'w') as f:
    f.write('static const char *pd_api_stub_paths[] = {\n')
    for p in sorted(paths):
        f.write('    "%s",\n' % p)
    f.write('    NULL\n};\n')
EOF
```

## Gotchas

- After editing any header in `src/`, the Makefile rebuilds all objects
  (headers are listed as prerequisites). If you ever see inexplicable
  memory corruption after a header change, `make clean && make`.
- Debug env vars: `PD_TRACE=1` (draw/sprite tracing), `PD_DUMP_FB=<frame>`
  (write framebuffer to /tmp/pd_fb.pbm), `PD_AUTO_A=<n>` (auto press A),
  `PD_MENU_PICK=<i>` / `PD_MENU_DUMP=1` (menu automation), `PD_TILT=x,y`
  (fake accelerometer after ~400 reads).
