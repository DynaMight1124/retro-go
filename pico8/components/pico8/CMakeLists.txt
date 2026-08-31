# pico8 component (PicoPico PICO-8 emulator) for the retro-go framework.
#
# This replaces the upstream PicoPico CMakeLists.txt. The component is
# compiled into the ESP-IDF application via base.cmake, which sets
# EXTRA_COMPONENT_DIRS = ${CMAKE_CURRENT_LIST_DIR}/../components.
#
# IMPORTANT — source layout and the unity-build trap:
#   src/engine.c text-includes (via raw #include "<name>.c") five other
#   sources in the same directory:
#       parser.c   synth.c   hud.c   sfx.c   pico8api.c
#   Each of those five sources must therefore NOT appear in SRCS, or we
#   get duplicate-symbol link errors (the same definitions compile twice,
#   once on their own TU and once inside engine.c via text-include).
#
# Therefore: src/*.c / src/*.cpp is NOT globbed. Standalone TUs are listed
# explicitly below. lua/*.cpp is globbed because Lua has no such recursion
# (each file is self-contained or includes only headers).
#
# NOTE on file(GLOB): do NOT pass CONFIGURE_DEPENDS here. ESP-IDF processes
# this file in a "script / find package" mode via
# component_get_requirements.cmake, where CONFIGURE_DEPENDS is rejected
# with: "CONFIGURE_DEPENDS is invalid for script and find package modes."
# The CMake cache will still invalidate when this CMakeLists.txt is
# modified, so the only cost of the missing flag is touching this file
# when a new lua/*.cpp is added.

cmake_minimum_required(VERSION 3.5)

# z8lua sources use C++17 (fix32, std::array, RAII helpers).
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Standalone TUs (one .c/.cpp each). Order is irrelevant because the
# linker will resolve all cross-TU references.
#
# Engine is a .cpp file (not .c) because it text-includes sfx.c, synth.c,
# and pico8api.c — three C files that consume the C++-only `z8::fix32` type
# from lua/fix32.h. Renaming engine.c -> engine.cpp lets CMake pick C++
# from the extension with no per-file property override needed. ESP-IDF's
# component-requirements script mode rejects both set_source_files_properties
# and CONFIGURE_DEPENDS.
set(PICO_STANDALONE_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/engine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/pico8_globals.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/retro_go_backend.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/p8_text_parser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/p8_png.cpp")

# Glob only the Lua tree — it has no text-include recursion.
file(GLOB PICOLUA_SRC
     "${CMAKE_CURRENT_SOURCE_DIR}/lua/*.cpp")

# Make our headers/data locatable from both pico8/main/ and the component.
set(COMPONENT_ADD_INCLUDEDIRS "."
                               "src"
                               "lua"
                               "stdlib"
                               "artifacts"
                               "src/generated")

# Direct ESP-IDF v5 registration. We bypass register_component() so the
# three source trees (src/ via explicit list, lua/ via glob) can be merged.
#
# REQUIRES retro-go: retro_go_backend.c includes <rg_audio.h> (for
# rg_audio_submit()), <rg_input.h>, etc. ESP-IDF propagates the dependency
# component's INCLUDE_DIRS into our include path so `#include <rg_audio.h>`
# resolves to components/retro-go/rg_audio.h.
idf_component_register(
    SRCS           "${PICO_STANDALONE_SRCS}" "${PICOLUA_SRC}"
    INCLUDE_DIRS   "${COMPONENT_ADD_INCLUDEDIRS}"
    REQUIRES       retro-go
)

# retro-go's per-app compile flags (-DRETRO_GO, jump-tables, PSRAM fix, ...).
# Must run AFTER idf_component_register so the flags attach to our targets.
#
# The target sdkconfig favours -Os globally, but Pico-8's measured bottleneck
# is its Lua interpreter and software rasterizer.  Reference Retro-Go cores
# override this locally as well.  Use -O2 rather than -O3: it restores
# inlining/vector-free loop optimisation without the larger instruction-cache
# footprint that can hurt the original ESP32. A measured P4-only -O3 pass also
# made Brutal Pico Race slower while increasing the binary by roughly 46 KB.
rg_setup_compile_options(-O2)
