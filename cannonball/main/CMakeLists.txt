set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS ".")
register_component()
rg_setup_compile_options()

# Keep the adapter's palette conversion and frame loop at the same optimization
# level as the Cannonball engine.
target_compile_options(${COMPONENT_LIB} PRIVATE -O3)
