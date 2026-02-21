idf_component_register(SRCS "main.c" 
                    INCLUDE_DIRS ".")

rg_setup_compile_options()

add_compile_definitions(ESP32_QUAKE_PAK_SIZE=0)
add_compile_definitions(ESP32_QUAKE_PAK_NAME=\"pak0.pak\")
