set(COMPONENT_ADD_INCLUDEDIRS 
    "."
    "engine"
    "font"
    "frontend"
    "hwaudio"
    "hwvideo"
    "res"
    "roms"
    "sdl"
    "thirdparty"
)

set(COMPONENT_SRCS 
    "engine/oanimseq.c"
    "engine/oattractai.c"
    "engine/obonus.c"
    "engine/ocrash.c"
    "engine/oentry.c"
    "engine/oferrari.c"
    "engine/ohiscore.c"
    "engine/ohud.c"
    "engine/oinitengine.c"
    "engine/oinputs.c"
    "engine/olevelobjs.c"
    "engine/ologo.c"
    "engine/omap.c"
    "engine/omusic.c"
    "engine/ooutputs.c"
    "engine/opalette.c"
    "engine/oroad.c"
    "engine/osmoke.c"
    "engine/osprite.c"
    "engine/osprites.c"
    "engine/ostats.c"
    "engine/otiles.c"
    "engine/otraffic.c"
    "engine/outils.c"
    "engine/outrun.c"
    "engine/audio/osound.c"
    "engine/audio/osoundint.c"
    "font/font_drawing.c"
    "frontend/config.c"
    "frontend/menu.c"
    "frontend/ttrial.c"
    "hwaudio/segapcm.c"
    "hwaudio/ym2151.c"
    "hwvideo/hwroad.c"
    "hwvideo/hwsprites.c"
    "hwvideo/hwtiles.c"
    "romloader.c"
    "roms.c"
    "sdl/audio.c"
    "sdl/input.c"
    "sdl/rendersw.c"
    "sdl/timer.c"
    "thirdparty/crc/crc.c"
    "thirdparty/sxmlc/sxmlc.c"
    "thirdparty/sxmlc/sxmlsearch.c"
    "trackloader.c"
    "utils.c"
    "video.c"
    "xmlutils.c"
    "cannonball_engine.c"
)

set(COMPONENT_REQUIRES "retro-go")

register_component()

rg_setup_compile_options(-Wno-misleading-indentation -Wno-dangling-else -Wno-type-limits -Wno-unused-but-set-variable -Wno-implicit-fallthrough -Wno-parentheses -Wno-unused-value -Wno-unused-const-variable -Wno-error=implicit-function-declaration -Wno-maybe-uninitialized -Wno-error=return-type -Wno-enum-int-mismatch -Wno-char-subscripts -Wno-format -Wno-incompatible-pointer-types)

target_compile_definitions(${COMPONENT_LIB} PRIVATE 
    -DCOMPILE_SOUND_CODE
    -DRETRO_GO
)

target_compile_options(${COMPONENT_LIB} PRIVATE "-include" "${CMAKE_CURRENT_SOURCE_DIR}/globals.h")
