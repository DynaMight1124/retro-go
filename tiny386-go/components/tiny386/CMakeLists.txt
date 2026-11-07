idf_component_register(
    SRCS i386.c i8042.c i8254.c i8259.c ide.c ini.c pc.c misc.c pci.c
         vga.c vgafont.c ne2000.c i8257.c sb16.c pcspk.c adlib.c fmopl.c t386_file.c
    INCLUDE_DIRS "."
    REQUIRES retro-go)
rg_setup_compile_options(-DBUILD_ESP32 -DBPP=16 -DUSE_FMOPL -DIRAM_ATTR_CPU_EXEC1=)
target_compile_options(${COMPONENT_LIB} PRIVATE -O2 -Wno-unused-function
    -Wno-unused-variable -Wno-unused-but-set-variable -Wno-sign-compare)

# Keep the tested interpreter switch policy fixed during the engine-wide O2
# comparison. Other sources retain Retro-Go's default switch optimizations.
set_source_files_properties(i386.c PROPERTIES COMPILE_OPTIONS
    "-fno-jump-tables;-fno-tree-switch-conversion")

# rg_setup_compile_options does not propagate the platform release definition.
# Keep RG_LOGD and test-only detailed instrumentation out of release firmware.
if(RG_BUILD_RELEASE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE RG_BUILD_RELEASE=1)
endif()
