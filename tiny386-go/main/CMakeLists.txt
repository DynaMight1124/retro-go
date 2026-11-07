idf_component_register(SRCS main.c t386_ui.c t386_controls.c t386_config.c t386_input.c t386_clock.c
    t386_audio_output.c t386_platform.c t386_runtime.c t386_firmware.c t386_embedded.c
    INCLUDE_DIRS "." REQUIRES retro-go tiny386
    EMBED_FILES "../firmware/bios.bin" "../firmware/vgabios.bin")
rg_setup_compile_options()

# rg_setup_compile_options does not propagate the platform release definition.
# Keep RG_LOGD and test-only detailed instrumentation out of release firmware.
if(RG_BUILD_RELEASE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE RG_BUILD_RELEASE=1)
endif()
