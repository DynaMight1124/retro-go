if(IDF_TARGET MATCHES "esp32p4|esp32s31")
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
        REQUIRES heap esp_mm esp_timer retro-go
    )

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
    # ESP32 and ESP32-S3 retain Retro-Go's established interpreter core.
    set(COMPONENT_SRCDIRS "orig")
    set(COMPONENT_ADD_INCLUDEDIRS "orig" "orig/libretro/libretro-common/include")
    set(COMPONENT_REQUIRES "retro-go")
    register_component()
    rg_setup_compile_options(
        -O3
        -fomit-frame-pointer -ffast-math
        -DOVERCLOCK_60FPS
        -DROM_BUFFER_SIZE=8
        -Wno-unused-variable
        -Wno-implicit-fallthrough
        -Wno-missing-field-initializers
        -Wno-error=unused-label
        -Wno-unused-label
    )
endif()
