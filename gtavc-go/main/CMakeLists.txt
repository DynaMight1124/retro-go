set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_REQUIRES "retro-go gtavc-go")

register_component()
if(RG_BUILD_RELEASE)
    component_compile_options(-DRG_BUILD_RELEASE=1)
endif()
rg_setup_compile_options(
    -O3
    -fomit-frame-pointer
    -finline-functions
    -fwrapv
    -Wno-format-truncation
    -Wno-error=format-truncation
    -Wno-error
)
