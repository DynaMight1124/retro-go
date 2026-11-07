#!/bin/sh
# Run with sh firmware/rebuild.sh on Linux/WSL. All outputs stay in this app.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
revision=81ec9ec0bcf45df11fb7f98339ec9036b546fca0
printf '%s  %s\n' 150cf902e7d37ead4ce5db96c777fe64f8da2ebb4e1844e91741c57d25faccbc "$root/source/seabios-$revision.tar.gz" | sha256sum -c -
mkdir -p "$root/build"
work=$(mktemp -d "$root/build/rebuild.XXXXXX")
tar -xzf "$root/source/seabios-$revision.tar.gz" -C "$work"
cd "$work/seabios-$revision"
patch -p1 < "$root/source/tiny386.patch"
cp "$root/source/tiny386.config" .config
# Avoid timestamps/hostnames in version strings on a clean toolchain.
printf '%s\n' "seabios-$revision" > .version
make olddefconfig
make -j2 EXTRAVERSION=-tiny386-go
cp out/bios.bin "$root/bios.bin"
cp out/vgabios.bin "$root/vgabios.bin"
cp .config "$root/source/resolved.config"
sha256sum "$root/bios.bin" "$root/vgabios.bin"
