#!/bin/bash
# Build Celeste ESP32 ELF binary

set -e

LIBGCC="$HOME/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/lib/gcc/xtensa-esp-elf/14.2.0/esp32s3/libgcc.a"

echo "Building celeste.elf..."

xtensa-esp32s3-elf-gcc \
  -O2 \
  -DESP_PLATFORM \
  -I local_include \
  -Dmain=app_main \
  -nostartfiles -nostdlib \
  -fPIC -shared \
  -fvisibility=hidden \
  -Wl,-e,app_main \
  -Wl,--gc-sections \
  main.c player.c level.c vfx.c gfx_esp32.c sprites.c \
  "$LIBGCC" -o celeste.elf

echo "Stripping..."
xtensa-esp32s3-elf-strip --strip-all --remove-section=.xt.prop \
  -o celeste-stripped.elf celeste.elf

echo "Potential required exports:"
xtensa-esp32s3-elf-nm -u celeste.elf
