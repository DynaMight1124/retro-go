#!/bin/bash
set -e
echo "Compiling tools..."
make -C tools

echo "Converting AIFF to AIFC..."
find sound/samples/ -name "*.aiff" | while read aiff; do
    base="${aiff%.aiff}"
    aifc="build/us/${base}.aifc"
    table="build/us/${base}.table"
    mkdir -p $(dirname "$aifc")
    tools/aiff_extract_codebook "$aiff" > "$table"
    tools/vadpcm_enc -c "$table" "$aiff" "$aifc"
done

echo "Assembling sound data..."
mkdir -p build/us/sound/sound_banks
python3 tools/assemble_sound.py build/us/sound/samples/ sound/sound_banks/ build/us/sound/sound_data.ctl build/us/sound/sound_data.tbl -DVERSION_US --endian little --bitwidth 32
python3 tools/assemble_sound.py --sequences build/us/sound/sequences.bin build/us/sound/bank_sets sound/sound_banks/ sound/sequences.json sound/sequences/us/*.m64 -DVERSION_US --endian little --bitwidth 32

echo "Converting to inc.c..."
hexdump -v -e '1/1 "0x%X,"' build/us/sound/sound_data.ctl > build/us/sound/sound_data.ctl.inc.c
hexdump -v -e '1/1 "0x%X,"' build/us/sound/sound_data.tbl > build/us/sound/sound_data.tbl.inc.c
hexdump -v -e '1/1 "0x%X,"' build/us/sound/sequences.bin > build/us/sound/sequences.bin.inc.c
hexdump -v -e '1/1 "0x%X,"' build/us/sound/bank_sets > build/us/sound/bank_sets.inc.c

echo "Copying headers to sound directory..."
cp build/us/sound/*.inc.c sound/

echo "Done!"
