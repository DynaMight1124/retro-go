set(PICO_DIR ".")

set(COMPONENT_SRCS
    ${PICO_DIR}/pico/pico.c
    ${PICO_DIR}/pico/cart.c
    ${PICO_DIR}/pico/memory.c
    ${PICO_DIR}/pico/state.c
    ${PICO_DIR}/pico/sek.c
    ${PICO_DIR}/pico/z80if.c
    ${PICO_DIR}/pico/videoport.c
    ${PICO_DIR}/pico/draw2.c
    ${PICO_DIR}/pico/draw.c
    ${PICO_DIR}/pico/mode4.c
    ${PICO_DIR}/pico/misc.c
    ${PICO_DIR}/pico/media.c
    ${PICO_DIR}/pico/eeprom.c
    ${PICO_DIR}/pico/patch.c
    ${PICO_DIR}/pico/sound/sound.c
    ${PICO_DIR}/pico/sound/resampler.c
    ${PICO_DIR}/pico/sound/sn76496.c
    ${PICO_DIR}/pico/sound/ym2612.c
    ${PICO_DIR}/pico/sound/mix.c
    ${PICO_DIR}/pico/carthw/carthw.c
    ${PICO_DIR}/pico/carthw/eeprom_spi.c
    ${PICO_DIR}/pico/carthw/svp/svp.c
    ${PICO_DIR}/pico/carthw/svp/memory.c
    ${PICO_DIR}/pico/carthw/svp/ssp16.c
    ${PICO_DIR}/cpu/fame/famec.c
    ${PICO_DIR}/cpu/cz80/cz80.c
    ${PICO_DIR}/unzip/unzip.c
    ${PICO_DIR}/zlib/adler32.c
    ${PICO_DIR}/zlib/crc32.c
    ${PICO_DIR}/zlib/inffast.c
    ${PICO_DIR}/zlib/inflate.c
    ${PICO_DIR}/zlib/inftrees.c
    ${PICO_DIR}/zlib/zutil.c
    ${PICO_DIR}/platform/retro-go/stubs.c
)

set(COMPONENT_ADD_INCLUDEDIRS 
    ${PICO_DIR}
    ${PICO_DIR}/pico
    ${PICO_DIR}/zlib
    ${PICO_DIR}/platform/retro-go
)

if(NOT CONFIG_IDF_TARGET_ESP32)
    list(APPEND COMPONENT_SRCS
        ${PICO_DIR}/pico/cd/cd_image.c
        ${PICO_DIR}/pico/cd/cd_parse.c
        ${PICO_DIR}/pico/cd/cdc.c
        ${PICO_DIR}/pico/cd/cdd.c
        ${PICO_DIR}/pico/cd/cell_map.c
        ${PICO_DIR}/pico/cd/gfx.c
        ${PICO_DIR}/pico/cd/gfx_dma.c
        ${PICO_DIR}/pico/cd/mcd.c
        ${PICO_DIR}/pico/cd/megasd.c
        ${PICO_DIR}/pico/cd/memory.c
        ${PICO_DIR}/pico/cd/misc.c
        ${PICO_DIR}/pico/cd/pcm.c
        ${PICO_DIR}/pico/cd/sek.c
        ${PICO_DIR}/pico/cd/libchdr/src/libchdr_bitstream.c
        ${PICO_DIR}/pico/cd/libchdr/src/libchdr_cdrom.c
        ${PICO_DIR}/pico/cd/libchdr/src/libchdr_chd.c
        ${PICO_DIR}/pico/cd/libchdr/src/libchdr_flac.c
        ${PICO_DIR}/pico/cd/libchdr/src/libchdr_huffman.c
        ${PICO_DIR}/pico/cd/libchdr/deps/lzma-24.05/src/Alloc.c
        ${PICO_DIR}/pico/cd/libchdr/deps/lzma-24.05/src/CpuArch.c
        ${PICO_DIR}/pico/cd/libchdr/deps/lzma-24.05/src/Delta.c
        ${PICO_DIR}/pico/cd/libchdr/deps/lzma-24.05/src/LzFind.c
        ${PICO_DIR}/pico/cd/libchdr/deps/lzma-24.05/src/LzmaDec.c
        ${PICO_DIR}/pico/cd/libchdr/deps/lzma-24.05/src/LzmaEnc.c
        ${PICO_DIR}/pico/cd/libchdr/deps/lzma-24.05/src/Sort.c
        ${PICO_DIR}/pico/cd/libchdr/deps/zstd-1.5.6/lib/common/entropy_common.c
        ${PICO_DIR}/pico/cd/libchdr/deps/zstd-1.5.6/lib/common/error_private.c
        ${PICO_DIR}/pico/cd/libchdr/deps/zstd-1.5.6/lib/common/fse_decompress.c
        ${PICO_DIR}/pico/cd/libchdr/deps/zstd-1.5.6/lib/common/xxhash.c
        ${PICO_DIR}/pico/cd/libchdr/deps/zstd-1.5.6/lib/common/zstd_common.c
        ${PICO_DIR}/pico/cd/libchdr/deps/zstd-1.5.6/lib/decompress/huf_decompress.c
        ${PICO_DIR}/pico/cd/libchdr/deps/zstd-1.5.6/lib/decompress/zstd_ddict.c
        ${PICO_DIR}/pico/cd/libchdr/deps/zstd-1.5.6/lib/decompress/zstd_decompress_block.c
        ${PICO_DIR}/pico/cd/libchdr/deps/zstd-1.5.6/lib/decompress/zstd_decompress.c
    )
    list(APPEND COMPONENT_ADD_INCLUDEDIRS 
        ${PICO_DIR}/pico/cd 
        ${PICO_DIR}/pico/cd/libchdr/include 
        ${PICO_DIR}/pico/cd/libchdr/deps/lzma-24.05/include
        ${PICO_DIR}/pico/cd/libchdr/deps/zstd-1.5.6/lib
    )
    set(CD_DEFINES "-DUSE_LIBCHDR=1" "-DZ7_ST" "-DZSTD_DISABLE_ASM")
else()
    set(CD_DEFINES "-DNO_CD")
endif()

set(COMPONENT_REQUIRES "retro-go")

idf_component_register(SRCS "${COMPONENT_SRCS}"
                       INCLUDE_DIRS "${COMPONENT_ADD_INCLUDEDIRS}"
                       REQUIRES "${COMPONENT_REQUIRES}")

# These defines are critical to enable the logic in draw.c, sound.c and pico_cmn.c
add_definitions(
    -DESP_PLATFORM
    -DEMU_F68K
    -D_USE_CZ80
    -DNO_SMS
    -DNO_32X
    -DPICO_NO_32X
    -DPICODRIVE_HACKS
    -DEL_LOG_CONTENTS=0
)

set(PICO_COMPILE_OPTIONS 
    -O3 
    -ffast-math
    $<$<STREQUAL:${CONFIG_IDF_TARGET_ARCH},xtensa>:-mtext-section-literals>
    -Wno-incompatible-pointer-types 
    -Wno-unused-variable 
    -Wno-unused-function 
    -Wno-implicit-fallthrough 
    -Wno-empty-body 
    -Wno-unused-but-set-variable 
    -Wno-unused-value 
    -Wno-format-overflow 
    -Wno-array-bounds
    -Wno-error=format
    -Wno-format
    -fno-jump-tables
    ${CD_DEFINES}
)

rg_setup_compile_options(${PICO_COMPILE_OPTIONS})

if(CONFIG_IDF_TARGET_ARCH STREQUAL "xtensa")
    # Optimize famec.c with -O2 instead of -O3 to keep function code size under 256KB
    # and prevent assembler "l32r out of range" errors due to huge literal pools.
    set_source_files_properties(${PICO_DIR}/cpu/fame/famec.c PROPERTIES COMPILE_FLAGS "-O2")
endif()

