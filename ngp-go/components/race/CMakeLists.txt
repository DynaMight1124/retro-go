set(COMPONENT_SRCS
    "cz80.c"
    "cz80_support.c"
    "flash.c"
    "graphics.c"
    "input.c"
    "neopopsound.c"
    "ngpBios.c"
    "race-memory.c"
    "sound.c"
    "state.c"
    "tlcs900h.c"
)
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_REQUIRES "retro-go")

register_component()

# These select public RACE header interfaces, so consumers such as main.cpp
# must compile with the same definitions as the core itself.
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    CZ80
    FRAMESKIP
    NGP_OPTIMIZATION_16BIT_READ
)

# RACE spends nearly all of its frame budget in the TLCS interpreter and
# scanline renderer. The reference Retro-Go interpreter cores use -O3 too;
# rg_setup_compile_options also supplies interpreter-friendly jump tables and
# the required original-ESP32 PSRAM cache workaround.
rg_setup_compile_options(
    -O3
    -Wno-error
    -Wno-unused-label
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-maybe-uninitialized
    -Wno-discarded-qualifiers
    -Wno-incompatible-pointer-types
)
