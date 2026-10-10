idf_component_register(SRCS "main.c" "video.c" "audio.c" "input.c" "port_logic.c" "lifecycle.c"
    "music_cd.c" "music_pcm.c" "music_esp32.c"
    INCLUDE_DIRS "." PRIV_REQUIRES retro-go quake2 esp_netif)
rg_setup_compile_options()
target_link_libraries(${COMPONENT_LIB} INTERFACE
    "-u QG_DrawFrame -u QG_SetPalette -u QG_GetKey -u QG_GetMouseMove -u snd_inited -u CDAudio_Init")
