# The upstream Frodo tree is intentionally kept pristine under upstream/.
# The selected coherent Frodo Lite subsystems live in src/core with only the
# embedded display, storage, preference and audio boundaries adapted.
set(FRODO_COMMON_SRCS
    "src/frodo_port.cpp"
    "src/core/C64.cpp"
    "src/core/Display.cpp"
    "src/core/Prefs.cpp"
    "src/core/SID.cpp"
    "src/core/IEC.cpp"
    "src/core/Tape.cpp"
    "src/core/CPU_common.cpp"
    "src/core/Cartridge.cpp"
    "src/core/1541gcr.cpp"
)

# Frodo Lite is the production engine on every supported ESP32 family. SC
# cannot sustain PAL speed even on ESP32-P4 (measured 35-39 fps at 100% CPU),
# while frameskip cannot recover simulation throughput. Keep an
# application-local override for explicit SC benchmarking and compatibility
# investigation.
option(FRODO_FORCE_SC "Build Frodo SC regardless of target" OFF)

if(FRODO_FORCE_SC)
    set(FRODO_TIMING_SRCS
        "src/core/CPUC64_SC.cpp"
        "src/core/VIC_SC.cpp"
        "src/core/CIA_SC.cpp"
        "src/core/CPU1541_SC.cpp"
        "src/core/VIA_SC.cpp"
    )
    set(FRODO_ENGINE_DEFINES FRODO_SC=1)
    # SC dispatches every individual C64 clock. O3's extra inlining and
    # switch optimization are worth the code-size tradeoff on this source set.
    set(FRODO_ENGINE_OPTIMIZATION -O3)
    message(STATUS "Frodo engine: SC (cycle exact)")
else()
    set(FRODO_TIMING_SRCS
        "src/core/CPUC64.cpp"
        "src/core/VIC.cpp"
        "src/core/CIA.cpp"
        "src/core/CPU1541.cpp"
        "src/core/VIA.cpp"
    )
    set(FRODO_ENGINE_DEFINES FRODO_LITE=1)
    set(FRODO_ENGINE_OPTIMIZATION -O2)
    message(STATUS "Frodo engine: Lite")
endif()

set(COMPONENT_SRCS ${FRODO_COMMON_SRCS} ${FRODO_TIMING_SRCS})

set(COMPONENT_ADD_INCLUDEDIRS
    "include"
    "src/core"
)

set(COMPONENT_REQUIRES "retro-go")

register_component()

rg_setup_compile_options(
    ${FRODO_ENGINE_OPTIMIZATION}
    -std=gnu++20
    -D${FRODO_ENGINE_DEFINES}
    -DPRECISE_CPU_CYCLES=1
    -DPRECISE_CIA_CYCLES=1
)
