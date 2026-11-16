set(COMPONENT_SRCDIRS "src/port src/platform")
set(COMPONENT_ADD_INCLUDEDIRS "src/port src/platform third_party")
set(COMPONENT_REQUIRES "retro-go")

register_component()
if(RG_BUILD_RELEASE)
    component_compile_options(-DRG_BUILD_RELEASE=1)
endif()

rg_setup_compile_options(
    -O3
    -fomit-frame-pointer
    -finline-functions
    -fwrapv
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-char-subscripts
    -Wno-missing-field-initializers
    -Wno-format-overflow
    -Wno-error=format-overflow
    -Wno-format-truncation
    -Wno-error=format-truncation
    -Wno-stringop-overflow
    -Wno-error=stringop-overflow
    -Wno-error
)
