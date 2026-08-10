idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES retro-go pocketsnes esp_hw_support)

target_compile_options(${COMPONENT_LIB} PRIVATE -D${RG_BUILD_TARGET}=1)
