# CMakeLists.txt for sm64-funkey component
# Generated for ESP-IDF build system

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(srcs
    # Core Game Source
    "src/game/area.c"
    "src/game/behavior_actions.c"
    "src/game/camera.c"
    "src/game/crash_screen.c"
    "src/game/debug.c"
    "src/game/debug_course.c"
    "src/game/envfx_bubbles.c"
    "src/game/envfx_snow.c"
    "src/game/game_init.c"
    "src/game/geo_misc.c"
    "src/game/hud.c"
    "src/game/ingame_menu.c"
    "src/game/interaction.c"
    "src/game/level_geo.c"
    "src/game/level_update.c"
    "src/game/macro_special_objects.c"
    "src/game/main.c"
    "src/game/mario.c"
    "src/game/mario_actions_airborne.c"
    "src/game/mario_actions_automatic.c"
    "src/game/mario_actions_cutscene.c"
    "src/game/mario_actions_moving.c"
    "src/game/mario_actions_object.c"
    "src/game/mario_actions_stationary.c"
    "src/game/mario_actions_submerged.c"
    "src/game/mario_misc.c"
    "src/game/mario_step.c"
    "src/game/memory.c"
    "src/game/moving_texture.c"
    "src/game/object_collision.c"
    "src/game/object_helpers.c"
    "src/game/object_list_processor.c"
    "src/game/obj_behaviors.c"
    "src/game/obj_behaviors_2.c"
    "src/game/paintings.c"
    "src/game/platform_displacement.c"
    "src/game/print.c"
    "src/game/profiler.c"
    "src/game/rendering_graph_node.c"
    "src/game/save_file.c"
    "src/game/screen_transition.c"
    "src/game/shadow.c"
    "src/game/skybox.c"
    "src/game/sound_init.c"
    "src/game/spawn_object.c"
    "src/game/spawn_sound.c"

    # Audio
    "src/audio/data.c"
    "src/audio/effects.c"
    "src/audio/external.c"
    "src/audio/globals_start.c"
    "src/audio/heap.c"
    "src/audio/load.c"
    "src/audio/playback.c"
    "src/audio/port_eu.c"
    "src/audio/seqplayer.c"
    "src/audio/synthesis.c"

    # Engine
    "src/engine/behavior_script.c"
    "src/engine/geo_layout.c"
    "src/engine/graph_node.c"
    "src/engine/graph_node_manager.c"
    "src/engine/level_script.c"
    "src/engine/math_util.c"
    "src/engine/surface_collision.c"
    "src/engine/surface_load.c"

    # PC Port Specifics
    "src/pc/configfile.c"
    "src/pc/fsutils.c"
    "src/pc/mixer.c"
    "src/pc/savestate.c"
    "src/pc/ultra_reimplementation.c"
    "src/pc/decompress.c"
    "src/pc/dummy_sound_data.c"
    "assets/mario_anims.c"
    "assets/demo_data.c"

    # Software Renderer (GFX)
    "src/pc/gfx/gfx_soft.c"
    "src/pc/gfx/gfx_cc.c"
    "src/pc/gfx/gfx_pc.c"
    "src/pc/gfx/gfx_sdl.c"

    # Data
    "data/behavior_data.c"

    # LibUltra Reimplemented / Needed
    "lib/src/guPerspectiveF.c"
    "lib/src/guLookAtRef.c"
    "lib/src/guOrthoF.c"
    "lib/src/guMtxF2L.c"
    "lib/src/guRotateF.c"
    "lib/src/guScaleF.c"
    "lib/src/guTranslateF.c"
    "lib/src/guNormalize.c"
    "lib/src/alBnkfNew.c"
    "lib/src/NaN.c"
    "lib/src/math/cosf.c"
    "lib/src/math/sinf.c"
    "lib/src/math/llconv.c"
    "lib/src/math/llmuldiv.c"
)

# Goddard
file(GLOB goddard_srcs "src/goddard/*.c")
file(GLOB goddard_dyn_srcs "src/goddard/dynlists/*.c")
list(APPEND srcs ${goddard_srcs} ${goddard_dyn_srcs})

# Menus
file(GLOB menu_srcs "src/menu/*.c")
list(APPEND srcs ${menu_srcs})

# Buffers
file(GLOB buffer_srcs "src/buffers/*.c")
list(APPEND srcs ${buffer_srcs})

# Segment Data (Actors, Levels, Bin)
file(GLOB_RECURSE actor_srcs "actors/*.c")
file(GLOB_RECURSE level_srcs "levels/*.c")
file(GLOB_RECURSE bin_srcs "bin/*.c")

set(data_srcs ${actor_srcs} ${level_srcs} ${bin_srcs})
list(FILTER data_srcs EXCLUDE REGEX ".*\\.inc\\.c")
list(FILTER data_srcs EXCLUDE REGEX "bin/eu/.*")
list(FILTER data_srcs EXCLUDE REGEX "bin/jp/.*")

list(APPEND srcs ${data_srcs})

set(skybox_names
    "bbh"
    "bidw"
    "bitfs"
    "bits"
    "ccm"
    "cloud_floor"
    "clouds"
    "ssl"
    "water"
    "wdw")

foreach(skybox_name IN LISTS skybox_names)
    set(skybox_png "${CMAKE_CURRENT_LIST_DIR}/textures/skyboxes/${skybox_name}.png")
    set(skybox_c "${CMAKE_CURRENT_LIST_DIR}/textures/skyboxes/${skybox_name}_skybox.c")
    if(EXISTS "${skybox_png}" AND NOT EXISTS "${skybox_c}")
        execute_process(
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_LIST_DIR}/tools/generate_skybox_c.py"
                    "${skybox_png}"
                    "${skybox_c}"
            RESULT_VARIABLE skybox_gen_result)
        if(NOT skybox_gen_result EQUAL 0)
            message(WARNING "Failed to generate ${skybox_c} from ${skybox_png}; skybox stubs may be used.")
        endif()
    endif()
endforeach()

file(GLOB generated_skybox_srcs "textures/skyboxes/*_skybox.c")
if(generated_skybox_srcs)
    list(APPEND srcs ${generated_skybox_srcs})
else()
    message(WARNING
        "SM64 skybox C sources were not found under textures/skyboxes/*_skybox.c; "
        "using ROM-clean stub pointer lists. Run extract_assets.py with a user ROM "
        "to generate local, ignored skybox C files before building if skyboxes should render.")
    list(APPEND srcs "src/pc/skybox_stubs.c")
endif()

# Register component
idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS 
        "include" 
        "src" 
        "src/pc" 
        "src/game" 
        "src/audio" 
        "src/engine" 
        "src/buffers"
        "."
    REQUIRES
        "retro-go"
)

# Compile options and definitions
component_compile_options(
    "-w"
    "-fno-strict-aliasing"
    "-Wno-incompatible-pointer-types"
    "-Wno-int-conversion"
    "-Wno-implicit-function-declaration"
)

component_compile_definitions(
    VERSION_US
    ENABLE_SOFTRAST=1
    CONVERT
    NON_MATCHING=1
    AVOID_UB=1
    TARGET_RS97=0
    TARGET_FUNKEY=1
    F3DEX_GBI_2E
    _LANGUAGE_C
    RETRO_GO=1
    NO_SEGMENTED_MEMORY=1
)
