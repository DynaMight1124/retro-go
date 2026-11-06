# Shared by the adapter, core and xjit: never select their ABIs separately.
# GBAJIT is deliberately an S3-only recovery switch; P4 behavior is unchanged.
if(IDF_TARGET STREQUAL "esp32p4" OR IDF_TARGET STREQUAL "esp32s31")
    set(GBSP_CORE "riscv")
elseif(IDF_TARGET STREQUAL "esp32s3" AND NOT "$ENV{GBAJIT}" STREQUAL "0")
    set(GBSP_CORE "s3")
else()
    set(GBSP_CORE "orig")
endif()
