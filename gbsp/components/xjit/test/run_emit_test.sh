#!/bin/bash
# Check xjit_emit.h against xtensa-esp32s3-elf-as: build emit_test on the host, assemble its
# text in the espressif/idf:v5.4 container, compare the bytes. Usage: run_emit_test.sh [seed]
set -e
cd "$(dirname "$0")"
T=$(mktemp -d)
gcc -O1 -Wall -Wno-unused-function -I../include emit_test.c -o $T/emit_test
$T/emit_test $T/t.S $T/t.bin $T/t.map ${1:-1}
docker run --rm -v $T:/t -w /t espressif/idf:v5.4 bash -c ". /opt/esp/idf/export.sh >/dev/null 2>&1; \
  xtensa-esp32s3-elf-as t.S -o t.o && xtensa-esp32s3-elf-objcopy -O binary -j .text t.o t.as.bin"
python3 - $T <<'PY'
import sys
t = sys.argv[1]
a = open(t + "/t.bin", "rb").read()
b = open(t + "/t.as.bin", "rb").read()
bad = 0
for line in open(t + "/t.map"):
    off, n, txt = line.split(" ", 2)
    off, n = int(off), int(n)
    if a[off:off + n] != b[off:off + n]:
        bad += 1
        if bad <= 15:
            print(f"MISMATCH {txt.strip():40s} xjit {a[off:off+n].hex()}  as {b[off:off+n].hex()}")
print(f"emitter vs assembler: {len(a)} / {len(b)} bytes, {bad} mismatching instructions")
sys.exit(1 if bad or len(a) != len(b) else 0)
PY
