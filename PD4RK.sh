#!/bin/bash

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi

source $controlfolder/control.txt

[ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"

get_controls

GAMEDIR=/$directory/ports/playdate4rk
CONFDIR="$GAMEDIR/conf"

mkdir -p "$GAMEDIR/conf"

cd "$GAMEDIR"

> "$GAMEDIR/log.txt" && exec > >(tee "$GAMEDIR/log.txt") 2>&1

export XDG_DATA_HOME="$CONFDIR"
export LD_LIBRARY_PATH="$GAMEDIR/libs.${DEVICE_ARCH}:$LD_LIBRARY_PATH"
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"

# gptokeyb no longer needed: the runtime reads the gamepad natively via SDL
# (left stick = crank, Select+Start = exit). ctrls.gptk kept as fallback:
#   $GPTOKEYB "playdate_runtime.${DEVICE_ARCH}" -c "$GAMEDIR/ctrls.gptk" &

pm_platform_helper "$GAMEDIR/playdate_runtime.${DEVICE_ARCH}"

# --integer-scale letterboxes to a crisp integer multiple; remove for stretch-to-fit
#./playdate_runtime.${DEVICE_ARCH} --fullscreen --integer-scale "$GAMEDIR"

./playdate_runtime.${DEVICE_ARCH} --fullscreen "$GAMEDIR"

pm_finish
