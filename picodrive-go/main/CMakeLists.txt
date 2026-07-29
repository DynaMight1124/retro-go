set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS ".")
register_component()
if(PICODRIVE_GO_PROFILE_LEVEL GREATER_EQUAL 2)
    rg_setup_compile_options(
        -DPPROF
        -DPPROF_LEVEL=${PICODRIVE_GO_PROFILE_LEVEL}
        -DPICODRIVE_PROFILE_LEVEL=${PICODRIVE_GO_PROFILE_LEVEL}
    )
else()
    rg_setup_compile_options(
        -DPICODRIVE_PROFILE_LEVEL=${PICODRIVE_GO_PROFILE_LEVEL}
    )
endif()
