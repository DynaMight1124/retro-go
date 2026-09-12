set(COMPONENT_SRCDIRS OpenTyrian SDL)
set(COMPONENT_ADD_INCLUDEDIRS OpenTyrian SDL)
set(COMPONENT_REQUIRES retro-go)

register_component()

rg_setup_compile_options(
    -Wno-error
    -Wno-format
    -Wno-error=address
    -Wno-dangling-pointer
    -Wno-array-bounds
    -Wno-restrict
    -Wno-implicit-fallthrough
    -Wno-absolute-value
    -Wno-attributes
    -Wno-misleading-indentation
    -Wno-format-overflow
    -Wno-char-subscripts
    -Wno-missing-field-initializers
    -Wno-format-truncation
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-unused-but-set-variable
    -Wno-stringop-truncation
    -O2
)
