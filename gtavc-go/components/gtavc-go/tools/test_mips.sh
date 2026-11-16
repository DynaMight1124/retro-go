#!/usr/bin/env bash
# Build the same host runner for MIPS32 soft-float and run it under qemu.
# This proves the port runs on a CPU without an FPU (all float goes through
# libgcc soft-float helpers).
set -euo pipefail
cd "$(dirname "$0")/.."
export GTAVC_SEED="${GTAVC_SEED:-1234567}"

M=${MIPS_PREFIX:-/home/Sajnaps/toolchains/mipsel-linux-muslsf-cross/bin/mipsel-linux-muslsf}
QEMU=${QEMU_MIPSEL:-/home/Sajnaps/toolchains/qemu/usr/bin/qemu-mipsel}
SYSROOT=${MIPS_SYSROOT:-/home/Sajnaps/toolchains/mipsel-linux-muslsf-cross/mipsel-linux-muslsf}

mkdir -p build screenshots_mips

${M}-gcc -EL -march=mips32 -mtune=mips32r2 -msoft-float -O2 -std=gnu11 -fwrapv -static -no-pie \
  -I src/platform -I src/port -I third_party \
  src/port/*.c src/platform/*.c runner/test_runner.c \
  -o build/gtavc_runner_mips -lm

${QEMU} -L "$SYSROOT" ./build/gtavc_runner_mips assets screenshots_mips
