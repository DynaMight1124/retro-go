#!/usr/bin/env bash
# Install GTA Vice City (J2ME C port) on a GB300 / SF2000 SD card.
# usage: ./install_to_sd.sh /media/user/GB300 [GB300V2|SF2000]
set -euo pipefail
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
DIST="$PROJECT_DIR/dist"
SD_ROOT="${1:-/media/Sajnaps/GB300}"
TYPE="${2:-GB300V2}"
CORE="$DIST/gtavc_${TYPE}.gb3k"
[ "$TYPE" = "SF2000" ] && CORE="$DIST/gtavc_SF2000.sf2k"

if [ ! -f "$CORE" ]; then
  echo "Brak $CORE - uruchom najpierw: make device FROGGY_TYPE=$TYPE"
  exit 1
fi
if [ ! -d "$SD_ROOT" ]; then
  echo "Nie znaleziono karty: $SD_ROOT"
  exit 1
fi

EXT=gb3k
[ "$TYPE" = "SF2000" ] && EXT=sf2k

echo "Instalacja GTA Vice City ($TYPE) w: $SD_ROOT"
mkdir -p "$SD_ROOT/system/Deimos/cores" \
         "$SD_ROOT/cores/gtavc" \
         "$SD_ROOT/ROMS/gtavc" \
         "$SD_ROOT/configs/custom_cores/gtavc" \
         "$SD_ROOT/bios"

# core in every place the loader looks
for e in gb3k sf2k; do
  cp -f "$CORE" "$SD_ROOT/system/Deimos/cores/gtavc.$e"
  cp -f "$CORE" "$SD_ROOT/cores/gtavc/gtavc.$e"
  cp -f "$CORE" "$SD_ROOT/ROMS/gtavc/gtavc.$e"
done
cp -f "$CORE" "$SD_ROOT/cores/gtavc/core_87000000"
cp -f "$CORE" "$SD_ROOT/ROMS/gtavc/core_87000000"

# game data next to the ROM file: the core resolves assets relative to it
echo "Kopiowanie danych gry (pełna mapa + cache, ~180 MB)..."
cp -a "$PROJECT_DIR/assets/." "$SD_ROOT/ROMS/gtavc/"

# The original S40 "legacy" configuration: 160x120 internal render, 5 ped
# models, 4 car models, 10 peds / 3 cars, 1-tile streaming ring.  This is the
# upstream game's own low-end mode, enabled by the presence of legacy.txt.
: > "$SD_ROOT/ROMS/gtavc/legacy.txt"

# Verify the critical data files (a partial copy is the usual reason for
# missing pedestrians / animation on the console).
missing=0
for f in mapidx.bin anim.bin timecyc.bin p_player.bin t_p_player.png \
         peds.lst cars.lst handling.txt wheel.bin w_bat.bin w_colt45.bin \
         font.png font_tip.png hud.png menu_bg.png radar.png radar.txt \
         intro.png ls0.png; do
  if [ ! -s "$SD_ROOT/ROMS/gtavc/$f" ]; then
    echo "BRAK: ROMS/gtavc/$f"
    missing=1
  fi
done
if [ ! -d "$SD_ROOT/ROMS/gtavc/t" ] || [ "$(ls "$SD_ROOT/ROMS/gtavc/t" | wc -l)" -lt 1000 ]; then
  echo "BRAK: kafli mapy w ROMS/gtavc/t"
  missing=1
fi
if [ ! -d "$SD_ROOT/ROMS/gtavc/x" ] || [ "$(ls "$SD_ROOT/ROMS/gtavc/x" | wc -l)" -lt 3000 ]; then
  echo "BRAK: tekstur mapy w ROMS/gtavc/x"
  missing=1
fi
if [ -d "$PROJECT_DIR/assets/tc" ]; then
  if [ ! -d "$SD_ROOT/ROMS/gtavc/tc" ] || [ "$(ls "$SD_ROOT/ROMS/gtavc/tc" | wc -l)" -lt 1000 ]; then
    echo "BRAK: cache kafli w ROMS/gtavc/tc (gra zadziała, ale streaming będzie wolniejszy)"
  fi
else
  echo "UWAGA: brak assets/tc - streaming na konsoli będzie wolniejszy"
fi
if [ -d "$PROJECT_DIR/assets/x" ] && [ "$(ls "$PROJECT_DIR/assets/x"/*.raw 2>/dev/null | wc -l)" -ge 3000 ]; then
  if [ "$(ls "$SD_ROOT/ROMS/gtavc/x"/*.raw 2>/dev/null | wc -l)" -lt 3000 ]; then
    echo "UWAGA: brak cache tekstur .raw - dekodowanie PNG może powodować zacięcia"
  fi
fi
if [ "$missing" != 0 ]; then
  echo "Kopiowanie danych niekompletne - powtórz instalację."
  exit 1
fi

printf 'gtavc;gtavc;GTAVC.gb3k' > "$SD_ROOT/ROMS/gtavc/GTAVC.gb3k"
printf 'gtavc;gtavc;GTAVC.sf2k' > "$SD_ROOT/ROMS/gtavc/GTAVC.sf2k"
cp -f "$SD_ROOT/ROMS/gtavc/GTAVC.gb3k" "$SD_ROOT/ROMS/GTAVC.gb3k"
cp -f "$SD_ROOT/ROMS/gtavc/GTAVC.sf2k" "$SD_ROOT/ROMS/GTAVC.sf2k"
printf 'GTAVC.gb3k|gtavc\nGTAVC.sf2k|gtavc\n' > "$SD_ROOT/configs/custom_cores/gtavc/games.opt"

# launcher artwork (reuse the upstream title shot if present)
if [ -f "$PROJECT_DIR/assets/menu_bg.png" ]; then
  mkdir -p "$SD_ROOT/ROMS/gtavc/.res" "$SD_ROOT/ROMS/.res"
  cp -f "$PROJECT_DIR/assets/menu_bg.png" "$SD_ROOT/ROMS/gtavc/.res/GTAVC.png"
  cp -f "$PROJECT_DIR/assets/menu_bg.png" "$SD_ROOT/ROMS/.res/GTAVC.png"
fi

sync
echo "Gotowe. Uruchom: ROMS/GTAVC.$EXT (albo ROMS/gtavc/GTAVC.$EXT)"
