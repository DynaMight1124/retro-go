set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS ".")
register_component()
rg_setup_compile_options()

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=address -Wno-unused-variable)
