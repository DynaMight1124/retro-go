#!/usr/bin/env bash
# Reproducible verification: build for host and for MIPS32 soft-float, replay
# the upstream Shots.java scenario with a fixed seed, and check that the
# deterministic screens are identical.
set -euo pipefail
cd "$(dirname "$0")/.."

SEED="${GTAVC_SEED:-1234567}"
M=${MIPS_PREFIX:-/home/Sajnaps/toolchains/mipsel-linux-muslsf-cross/bin/mipsel-linux-muslsf}
QEMU=${QEMU_MIPSEL:-/home/Sajnaps/toolchains/qemu/usr/bin/qemu-mipsel}
SYSROOT=${MIPS_SYSROOT:-/home/Sajnaps/toolchains/mipsel-linux-muslsf-cross/mipsel-linux-muslsf}

mkdir -p build screenshots screenshots_mips
make host >/dev/null

echo "== host run =="
rm -rf /tmp/gtavc_port_test
GTAVC_SEED="$SEED" ./build/gtavc_runner assets screenshots >/tmp/gtavc_host.log 2>&1

echo "== MIPS32 soft-float run (qemu) =="
${M}-gcc -EL -march=mips32 -mtune=mips32r2 -msoft-float -O2 -std=gnu11 -fwrapv -static -no-pie \
  -I src/platform -I src/port -I third_party \
  src/port/*.c src/platform/*.c runner/test_runner.c -o build/gtavc_runner_mips -lm
rm -rf /tmp/gtavc_port_test
GTAVC_SEED="$SEED" ${QEMU} -L "$SYSROOT" ./build/gtavc_runner_mips assets screenshots_mips >/tmp/gtavc_mips.log 2>&1

echo "== simulation log =="
if diff -q <(grep -vE '^shot|ms/frame|===|FRAME|STATE|PED |CAR |PK |BROKEN' /tmp/gtavc_host.log) \
           <(grep -vE '^shot|ms/frame|===|FRAME|STATE|PED |CAR |PK |BROKEN' /tmp/gtavc_mips.log) >/dev/null; then
  echo "PASS: host and MIPS simulation output is identical"
else
  echo "FAIL: simulation output differs"
  exit 1
fi

echo "== deterministic UI screens =="
for n in 00_splash 01_menu 03_map; do
  if cmp -s "screenshots/$n.ppm" "screenshots_mips/$n.ppm"; then
    echo "PASS: $n identical"
  else
    echo "FAIL: $n differs"
    exit 1
  fi
done

echo "== MIPS soft-float ABI =="
${M}-readelf -A build/gtavc_runner_mips | grep -q "Soft float" && echo "PASS: Tag_GNU_MIPS_ABI_FP = Soft float"
if ${M}-objdump -d build/gtavc_runner_mips | grep -qE '\b(cop1|mtc1|mfc1|lwc1|swc1|ldc1|sdc1)\b'; then
  echo "FAIL: FPU instructions found"
  exit 1
fi
echo "PASS: no FPU instructions"

echo "All checks passed."
