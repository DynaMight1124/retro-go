# Audio banks and sample tables (raw big-endian ROM data, same bytes the N64
# build links).  data/sound_data/audiobanks.s and audiotables.s carry no
# labels, so the port includes the blobs here under names segments.c aliases
# to _audio_banksSegmentRomStart / _audio_tablesSegmentRomStart.
.include "macros.inc"

.section .data

.balign 16
glabel port_audiobanks
.incbin "bin/audiobanks.us.bin"

.balign 16
glabel port_audiotables
.incbin "bin/audiotables.bin"
