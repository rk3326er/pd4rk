# Playdate Runtime for PortMaster
# Cross-compilable C project. Lua 5.4.3 vendored and patched.

CC ?= cc
CFLAGS = -std=c11 -Wall -Wno-unused-function -O2 -Isrc -Ivendor/lua
LDFLAGS = -lm -lz -ldl -lpthread

# --- macOS native build ---
ifeq ($(shell uname -s), Darwin)
	SDL2_CFLAGS = $(shell sdl2-config --cflags 2>/dev/null || echo "-I/opt/homebrew/include -I/usr/local/include")
	SDL2_LIBS   = $(shell sdl2-config --libs 2>/dev/null || echo "-L/opt/homebrew/lib -L/usr/local/lib -lSDL2 -lSDL2_mixer")
	SDKROOT    ?= $(shell xcrun --show-sdk-path 2>/dev/null)
	CFLAGS     += -DPLATFORM_MACOS
else
	SDL2_CFLAGS = $(shell sdl2-config --cflags 2>/dev/null || echo "$(shell pkg-config --cflags sdl2 2>/dev/null)")
	SDL2_LIBS   = $(shell sdl2-config --libs 2>/dev/null || echo "$(shell pkg-config --libs sdl2 2>/dev/null)") $(shell pkg-config --libs SDL2_mixer 2>/dev/null || echo "-lSDL2_mixer")
endif

# --- PortMaster aarch64 cross-compile ---
ifeq ($(CROSS), aarch64)
	CC = aarch64-linux-gnu-gcc
	SDL2_CFLAGS = $(shell aarch64-linux-gnu-pkg-config --cflags sdl2 SDL2_mixer 2>/dev/null || echo "-I/usr/aarch64-linux-gnu/include/SDL2")
	SDL2_LIBS   = $(shell aarch64-linux-gnu-pkg-config --libs sdl2 SDL2_mixer 2>/dev/null || echo "-L/usr/aarch64-linux-gnu/lib -lSDL2 -lSDL2_mixer")
	CFLAGS     += -DLUA_32BITS
endif

CFLAGS += -DLUA_32BITS -DLUA_USE_LINUX -D_GNU_SOURCE $(SDL2_CFLAGS)

LUA_SRC = $(wildcard vendor/lua/lapi.c vendor/lua/lauxlib.c vendor/lua/lbaselib.c \
	vendor/lua/lcode.c vendor/lua/lcorolib.c vendor/lua/lctype.c vendor/lua/ldblib.c \
	vendor/lua/ldebug.c vendor/lua/ldo.c vendor/lua/ldump.c vendor/lua/lfunc.c \
	vendor/lua/lgc.c vendor/lua/llex.c vendor/lua/lmem.c vendor/lua/lobject.c \
	vendor/lua/lopcodes.c vendor/lua/lparser.c vendor/lua/lstate.c vendor/lua/lstring.c \
	vendor/lua/ltable.c vendor/lua/ltm.c vendor/lua/lundump.c vendor/lua/lvm.c \
	vendor/lua/lzio.c vendor/lua/lstrlib.c vendor/lua/ltablib.c vendor/lua/loadlib.c \
	vendor/lua/liolib.c vendor/lua/lutf8lib.c vendor/lua/linit.c \
	vendor/lua/lmathlib.c vendor/lua/loslib.c)

RUNTIME_SRC = $(wildcard src/*.c)

OBJ = $(LUA_SRC:.c=.o) $(RUNTIME_SRC:.c=.o)

TARGET = playdate_runtime

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(SDL2_LIBS)

%.o: %.c $(wildcard src/*.h)
	$(CC) $(CFLAGS) -c -o $@ $<

test-lua: $(LUA_SRC:.c=.o)
	@echo "Lua objects compiled successfully"

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean test-lua
