include("${CMAKE_CURRENT_LIST_DIR}/core_selection.cmake")

# All CPU backends compile the same portable core and public headers.
set(GPSP_SRCS
    cheats.c
    cpu.cpp
    gba_cc_lut.c
    gpsp_persistence.c
    gba_memory.c
    gbp.c
    input.c
    main.c
    memmap.c
    rfu.c
    savestate.c
    serial.c
    sound.c
    video.cpp

)
set(GPSP_REQUIRES retro-go)
if(GBSP_CORE STREQUAL "riscv")
    list(APPEND GPSP_SRCS cpu_threaded.c gpsp_esp.c gpsp_memory_alloc.c
         serial_proto.c gpsp_rom_stream.c bios_data.S riscv/riscv_stub.S)
    list(APPEND GPSP_REQUIRES heap esp_mm esp_timer)
elseif(GBSP_CORE STREQUAL "s3")
    list(APPEND GPSP_SRCS cpu_threaded.c xtensa/xtensa_stub.c xtensa/xt_fastmem.S)
    list(APPEND GPSP_REQUIRES xjit esp_mm)
endif()

idf_component_register(SRCS ${GPSP_SRCS}
    INCLUDE_DIRS "." "libretro/libretro-common/include"
    REQUIRES ${GPSP_REQUIRES})

if(GBSP_CORE STREQUAL "riscv")
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        COLOR_16_BIT
        COLOR_5_6_5
    )

    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        NDEBUG
        HAVE_STRLCPY
        ESP_PLATFORM
        SMALL_TRANSLATION_CACHE
        MMAP_JIT_CACHE
        RISCV_ARCH
        HAVE_DYNAREC
        HAVE_JIT
        ROM_BUFFER_SIZE=32
    )

    # Keep warning suppression scoped to the new third-party dynarec core.
    target_compile_options(${COMPONENT_LIB} PRIVATE
        -w
        -O3
        -fomit-frame-pointer
        -ffast-math
        -fno-exceptions
        -fno-rtti
        -msmall-data-limit=0
    )

    # Pass the component directory to the assembler so bios_data.S can resolve its .incbin.
    set_source_files_properties(bios_data.S PROPERTIES COMPILE_FLAGS "-Wa,-I${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(${COMPONENT_LIB} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:-Wa,-I${CMAKE_CURRENT_SOURCE_DIR}>")

    # Prevent C++ from pulling in libstdc++ (templates are compile-time only).
    idf_build_set_property(LINK_OPTIONS "-nostdlib++" APPEND)

    # irom_seg and drom_seg intentionally share a VMA through different flash buses.
    idf_build_set_property(LINK_OPTIONS "-Wl,--no-check-sections" APPEND)
else()
    rg_setup_compile_options(
        -O3 -fomit-frame-pointer -ffast-math
        -DOVERCLOCK_60FPS -DROM_BUFFER_SIZE=8
        -Wno-unused-variable -Wno-implicit-fallthrough
        -Wno-missing-field-initializers -Wno-error=unused-label -Wno-unused-label
    )
    if(GBSP_CORE STREQUAL "s3")
        # These definitions also control layout in the adapter's shared headers.
        target_compile_definitions(${COMPONENT_LIB} PUBLIC
            HAVE_DYNAREC XTENSA_ARCH SMALL_TRANSLATION_CACHE)
        set_source_files_properties(cpu_threaded.c PROPERTIES
            COMPILE_OPTIONS "-Os;-mtext-section-literals")
    endif()
endif()

# Retro-Go log macros are compiled in this component, not in the platform library.
target_compile_definitions(${COMPONENT_LIB} PRIVATE RG_BUILD_RELEASE=$<BOOL:${RG_BUILD_RELEASE}>)
