set(pocketsnes_srcs
    "snes9x/apu.cpp"
    "snes9x/c4.cpp"
    "snes9x/c4emu.cpp"
    "snes9x/cheats.cpp"
    "snes9x/cheats2.cpp"
    "snes9x/clip.cpp"
    "snes9x/cpu.cpp"
    "snes9x/cpuexec.cpp"
    "snes9x/cpuops.cpp"
    "snes9x/data.cpp"
    "snes9x/dma.cpp"
    "snes9x/dsp1.cpp"
    "snes9x/fxemu.cpp"
    "snes9x/fxinst.cpp"
    "snes9x/gfx.cpp"
    "snes9x/globals.cpp"
    "snes9x/memmap.cpp"
    "snes9x/movie.cpp"
    "snes9x/obc1.cpp"
    "snes9x/ppu.cpp"
    "snes9x/sa1.cpp"
    "snes9x/sa1cpu.cpp"
    "snes9x/screenshot.cpp"
    "snes9x/sdd1.cpp"
    "snes9x/sdd1emu.cpp"
    "snes9x/seta.cpp"
    "snes9x/seta010.cpp"
    "snes9x/seta011.cpp"
    "snes9x/seta018.cpp"
    "snes9x/snaporig.cpp"
    "snes9x/snapshot.cpp"
    "snes9x/soundux.cpp"
    "snes9x/spc700.cpp"
    "snes9x/spc7110.cpp"
    "snes9x/spc7110dec.cpp"
    "snes9x/srtc.cpp"
    "snes9x/tile.cpp"
)

idf_component_register(SRCS ${pocketsnes_srcs}
                       INCLUDE_DIRS "include" "linux" "snes9x"
                       PRIV_REQUIRES retro-go)

target_compile_definitions(${COMPONENT_LIB} PUBLIC
    __LINUX__
    RC_OPTIMIZED
    FOREVER_16_BIT
    SPC700_C
    RIGHTSHIFT_IS_SAR
    FAST_LSB_WORD_ACCESS
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -O3
    -fomit-frame-pointer
    -fpermissive
    -fno-strict-aliasing
    -fjump-tables
    -ftree-switch-conversion
    -w
)
