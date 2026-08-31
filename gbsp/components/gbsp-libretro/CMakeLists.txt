set(GPSP_SRCS
    gpsp_main.c
    gba_memory.c
    gba_cc_lut.c
    memmap.c
    sound.c
    input.c
    cheats.c
    serial.c
    serial_proto.c
    rfu.c
    gbp.c
    savestate.c
    gpsp_esp.c
    gpsp_memory_alloc.c
    bios_data.S
    cpu_threaded.c
    riscv/riscv_stub.S
)

set(GPSP_CXX_SRCS
    cpu.cc
    video.cc
)

idf_component_register(
    SRCS ${GPSP_SRCS} ${GPSP_CXX_SRCS}
    INCLUDE_DIRS "."
    REQUIRES heap esp_mm esp_timer
)

# gpSP compile definitions
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    COLOR_16_BIT
    COLOR_5_6_5
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    NDEBUG
    HAVE_STRLCPY
    ESP_PLATFORM
    SMALL_TRANSLATION_CACHE
)

if(IDF_TARGET MATCHES "esp32p4|esp32s31")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        MMAP_JIT_CACHE
        RISCV_ARCH
        HAVE_DYNAREC
        HAVE_JIT
        ROM_BUFFER_SIZE=32
    )
else()
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        ROM_BUFFER_SIZE=8
    )
endif()

# Suppress warnings in third-party code; no-exceptions/rtti to avoid libstdc++ bloat
target_compile_options(${COMPONENT_LIB} PRIVATE -w -O3 -fomit-frame-pointer -ffast-math -fno-exceptions -fno-rtti -msmall-data-limit=0)

# Prevent C++ from pulling in libstdc++ (templates are compile-time only)
idf_build_set_property(LINK_OPTIONS "-nostdlib++" APPEND)

# Disable section overlap check — irom_seg and drom_seg share the same VMA
# by design (different bus ports to the same flash), and --enable-non-contiguous-regions
# incorrectly rejects this as an overlap
idf_build_set_property(LINK_OPTIONS "-Wl,--no-check-sections" APPEND)
