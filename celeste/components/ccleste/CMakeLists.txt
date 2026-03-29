set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_REQUIRES retro-go)

register_component()

if(RG_BUILD_TARGET)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC ${RG_BUILD_TARGET})
endif()
