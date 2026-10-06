idf_component_register(
    SRCS "rom.c" "mio0.c" "assets.c" "resident.c" "recipe_index.c"
         "controller_rg.c" "preview.c"
         "course_relocs.c" "course_segments.c"
         "unpack_shim.c"
         "src/port/ultra_shim.c"
         "src/port/decoders.c" "tools/libtkmk00.c"
         "src/port/rg/game_memory.c"
         "src/port/rg/menu_cache.c"
         "src/port/rg/port_rg.c"
         "src/port/rg/gfx_rg.c"
         "src/port/rg/audio_rg.c" "src/port/audio/mixer.c" "src/port/audio/mix_jobs.c" "src/buffers/audio_heap.c" "src/port/rg/menu_data.c"
         "src/buffers/buffers.c" "src/buffers/gfx_output_buffer.c"
         "src/buffers/random.c"
         "src/buffers/trig_tables.c" "src/data/kart_attributes.c"
         "src/os/guLookAtF.c" "src/os/guLookAtRef.c"
         "src/os/guMtxCatF.c" "src/os/guMtxCatL.c"
         "src/os/guMtxF2L.c" "src/os/guNormalize.c"
         "src/os/guOrthoF.c" "src/os/guPerspectiveF.c"
         "src/os/guRotateF.c" "src/os/guScaleF.c"
         "src/os/guTranslateF.c"
         "asset_aliases.S"
    INCLUDE_DIRS "." "include" "src" "src/racing" "src/ending" "tools"
                 "src/port" "src/port/gfx" "src/port/rg" "courses"
    REQUIRES "retro-go" "mbedtls"
)

rg_setup_compile_options()

# Retro-Go sets this privately on its own component. Propagate the selected
# build mode to the game adapter and application so RG_LOGD compiles out.
target_compile_definitions(${COMPONENT_LIB} PUBLIC RG_BUILD_RELEASE=$<BOOL:${RG_BUILD_RELEASE}>)

# IDF supplies -Os globally. This application spends nearly all its runtime
# in CPU rasterization; explicitly optimize the engine component for speed.
# Keep normal floating-point semantics (no fast-math).
# Actors and game objects intentionally overlay several structs in fixed pools.
# Their N64 memory contract requires aliasing; tell the optimizer explicitly.
target_compile_options(${COMPONENT_LIB} PRIVATE -O3 -fno-strict-aliasing)

option(MK64_RG_PROFILE "Enable detailed MK64 CPU and renderer serial profiling" OFF)
option(MK64_RG_MENU_TIMING "Enable targeted menu loading and audio wait timing summaries" OFF)
target_compile_definitions(${COMPONENT_LIB} PUBLIC MK64_RG_MENU_TIMING=$<BOOL:${MK64_RG_MENU_TIMING}>)
target_compile_definitions(${COMPONENT_LIB} INTERFACE MK64_RG_PROFILE=$<BOOL:${MK64_RG_PROFILE}>)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    VERSION_US=1 F3DEX_GBI=1 F3D_OLD=1 NON_MATCHING=1 AVOID_UB=1
    NO_SEGMENTED_MEMORY=1 PORT_DRAW_DIST=3000 PORT_ENABLE_AUDIO=1 PORT_NO_WIDE_FOV=1
    MK64_RG_COURSE_TRACE=0
    MK64_RG_PROFILE=$<BOOL:${MK64_RG_PROFILE}>
    MK64_RG_PHASE_PROFILE=$<BOOL:${MK64_RG_PROFILE}>
)

# The PSP link script formed these two pointers from adjacent ROM assets.
# The asset recipe named the start of the 7ED50C blob as the preceding end.
target_link_options(${COMPONENT_LIB} INTERFACE
    "-Wl,--defsym=D_0B002A00=texture_player_select+0x2A00"
    "-Wl,--defsym=gTexture7ED50C=gTexture7ED290_end")

# Some libultra math files include PR/gu.h directly; that header assumes the
# complete ultra64 type set has already been included.
file(GLOB MK64_GU_SOURCES "src/os/gu*.c")
set_source_files_properties(${MK64_GU_SOURCES} PROPERTIES COMPILE_OPTIONS
    "-include;ultra64.h")

# Keep the original game and menu code compiling while integration is staged.
# The crash screen includes a generated font that is not in the source tree.
file(GLOB MK64_GAME_SOURCES "src/*.c" "src/racing/*.c" "src/ending/*.c" "src/audio/*.c")
list(REMOVE_ITEM MK64_GAME_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/crash_screen.c")
target_sources(${COMPONENT_LIB} PRIVATE ${MK64_GAME_SOURCES})
