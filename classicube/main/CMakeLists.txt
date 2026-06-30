set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS ".")
idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES retro-go ClassiCube)

rg_setup_compile_options()
