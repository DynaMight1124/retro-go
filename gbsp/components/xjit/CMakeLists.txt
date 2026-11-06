# This component is private to gpSP's ESP32-S3 backend.
include("${CMAKE_CURRENT_LIST_DIR}/../gbsp-libretro/core_selection.cmake")
if(GBSP_CORE STREQUAL "s3")
    idf_component_register(SRCS "xjit_block.c" "xjit_exec.c"
                          INCLUDE_DIRS "include"
                          PRIV_REQUIRES esp_mm heap)
else()
    idf_component_register()
endif()
