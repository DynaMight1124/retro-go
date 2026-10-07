set(COMPONENT_SRCS
    "libpcsxcore/cdriso.c"
    "libpcsxcore/cdrom.c"
    "libpcsxcore/cdrom-async.c"
    "libpcsxcore/cheat.c"
    "libpcsxcore/database.c"
    "libpcsxcore/decode_xa.c"
    "libpcsxcore/mdec.c"
    "libpcsxcore/misc.c"
    "libpcsxcore/plugins.c"
    "libpcsxcore/ppf.c"
    "libpcsxcore/psxbios.c"
    "libpcsxcore/psxcommon.c"
    "libpcsxcore/psxcounters.c"
    "libpcsxcore/psxdma.c"
    "libpcsxcore/psxhw.c"
    "libpcsxcore/psxinterpreter.c"
    "libpcsxcore/psxmem.c"
    "libpcsxcore/psxevents.c"
    "libpcsxcore/r3000a.c"
    "libpcsxcore/sio.c"
    "libpcsxcore/spu.c"
    "libpcsxcore/gpu.c"
    "libpcsxcore/gte.c"
    "libpcsxcore/gte_nf.c"
    "libpcsxcore/gte_divider.c"
    "deps/libchdr/src/libchdr_bitstream.c"
    "deps/libchdr/src/libchdr_cdrom.c"
    "deps/libchdr/src/libchdr_chd.c"
    "deps/libchdr/src/libchdr_flac.c"
    "deps/libchdr/src/libchdr_huffman.c"
    "deps/libchdr/deps/lzma-24.05/src/LzmaDec.c"
    "deps/libchdr/deps/lzma-24.05/src/Alloc.c"
    "deps/libchdr/deps/lzma-24.05/src/CpuArch.c"
    "deps/libchdr/deps/lzma-24.05/src/LzmaEnc.c"
    "deps/libchdr/deps/lzma-24.05/src/LzFind.c"
    "deps/libchdr/deps/zstd-1.5.6/lib/common/debug.c"
    "deps/libchdr/deps/zstd-1.5.6/lib/common/entropy_common.c"
    "deps/libchdr/deps/zstd-1.5.6/lib/common/error_private.c"
    "deps/libchdr/deps/zstd-1.5.6/lib/common/fse_decompress.c"
    "deps/libchdr/deps/zstd-1.5.6/lib/common/pool.c"
    "deps/libchdr/deps/zstd-1.5.6/lib/common/threading.c"
    "deps/libchdr/deps/zstd-1.5.6/lib/common/xxhash.c"
    "deps/libchdr/deps/zstd-1.5.6/lib/common/zstd_common.c"
    "deps/libchdr/deps/zstd-1.5.6/lib/decompress/huf_decompress.c"
    "deps/libchdr/deps/zstd-1.5.6/lib/decompress/zstd_ddict.c"
    "deps/libchdr/deps/zstd-1.5.6/lib/decompress/zstd_decompress.c"
    "deps/libchdr/deps/zstd-1.5.6/lib/decompress/zstd_decompress_block.c"
    "deps/libchdr/deps/zlib-1.3.1/adler32.c"
    "deps/libchdr/deps/zlib-1.3.1/compress.c"
    "deps/libchdr/deps/zlib-1.3.1/crc32.c"
    "deps/libchdr/deps/zlib-1.3.1/deflate.c"
    "deps/libchdr/deps/zlib-1.3.1/infback.c"
    "deps/libchdr/deps/zlib-1.3.1/inffast.c"
    "deps/libchdr/deps/zlib-1.3.1/inflate.c"
    "deps/libchdr/deps/zlib-1.3.1/inftrees.c"
    "deps/libchdr/deps/zlib-1.3.1/trees.c"
    "deps/libchdr/deps/zlib-1.3.1/uncompr.c"
    "deps/libchdr/deps/zlib-1.3.1/zutil.c"
    "plugins/dfsound/dma.c"
    "plugins/dfsound/freeze.c"
    "plugins/dfsound/registers.c"
    "plugins/dfsound/spu.c"
    "plugins/dfsound/out.c"
    "plugins/dfsound/retro_go.c"
    "plugins/gpu_unai/gpulib_if.cpp"
    "plugins/gpu_unai/old/if.cpp"
    "plugins/gpulib/gpu.c"
    "plugins/gpulib/prim.c"
    "plugins/gpulib/vout_pl.c"
    "frontend/cspace.c"
    "retro_go_stubs.c"
)

set(PCSX_NDRC_RV32_DEFAULT OFF)
option(PCSX_DUAL_DYNAREC "Include Lightrec and the experimental RV32 core in the P4 app" ON)
# The old single-core RV32 build remains available for isolated diagnostics.
option(PCSX_NDRC_RV32_SELFTEST "Run experimental RV32 emitter tests before Lightrec initialization" ${PCSX_NDRC_RV32_DEFAULT})
option(PCSX_NDRC_RV32_LIVE_CANARY "Execute the validated RV32 core with Lightrec retained as fallback" ${PCSX_NDRC_RV32_DEFAULT})
option(PCSX_NDRC_RV32_FULL_CORE "Replace Lightrec with the Ari64 RV32 new_dynarec" ${PCSX_NDRC_RV32_DEFAULT})
option(PCSX_NDRC_RV32_DIFF "Enable slow per-instruction RV32 differential tracing" OFF)
option(PCSX_NDRC_RV32_PROFILE "Sample RV32 new_dynarec GTE costs every 60 frames" OFF)
option(PCSX_PORT_PROFILE "Enable detailed PCSX frame and renderer diagnostics" OFF)
if(PCSX_NDRC_RV32_FULL_CORE)
    set(PCSX_DUAL_DYNAREC OFF)
endif()

if(CONFIG_IDF_TARGET_ESP32P4)
    if(PCSX_NDRC_RV32_SELFTEST AND NOT PCSX_NDRC_RV32_FULL_CORE AND NOT PCSX_DUAL_DYNAREC)
        list(APPEND COMPONENT_SRCS
            "libpcsxcore/new_dynarec/rv32/selftest.c"
            "libpcsxcore/new_dynarec/rv32/oracle.S"
            "libpcsxcore/new_dynarec/rv32/linkage.S"
            "libpcsxcore/new_dynarec/rv32/abi_test.c"
            "libpcsxcore/new_dynarec/rv32/abi_test.S"
            "libpcsxcore/new_dynarec/rv32/flags_test.c"
            "libpcsxcore/new_dynarec/rv32/compiler_test.c"
            "libpcsxcore/new_dynarec/rv32/block_cache.c"
        )
    endif()
    if(PCSX_NDRC_RV32_SELFTEST OR PCSX_NDRC_RV32_FULL_CORE OR PCSX_DUAL_DYNAREC)
        list(APPEND COMPONENT_SRCS "libpcsxcore/new_dynarec/rv32/esp32p4.c")
    endif()
    if(PCSX_NDRC_RV32_FULL_CORE OR PCSX_DUAL_DYNAREC)
        list(APPEND COMPONENT_SRCS
            "libpcsxcore/new_dynarec/new_dynarec.c"
            "libpcsxcore/new_dynarec/emu_if.c"
            "libpcsxcore/new_dynarec/pcsxmem.c"
            "libpcsxcore/new_dynarec/linkage_rv32.c"
            "libpcsxcore/new_dynarec/linkage_rv32_full.S"
        )
    endif()
    if(NOT PCSX_NDRC_RV32_FULL_CORE OR PCSX_DUAL_DYNAREC)
        list(APPEND COMPONENT_SRCS
            "libpcsxcore/lightrec/plugin.c"
            "deps/lightrec/blockcache.c"
            "deps/lightrec/constprop.c"
            "deps/lightrec/disassembler.c"
            "deps/lightrec/emitter.c"
            "deps/lightrec/interpreter.c"
            "deps/lightrec/lightrec.c"
            "deps/lightrec/memmanager.c"
            "deps/lightrec/optimizer.c"
            "deps/lightrec/regcache.c"
            "deps/lightrec/tlsf/tlsf.c"
            "deps/lightning/lib/jit_disasm.c"
            "deps/lightning/lib/jit_memory.c"
            "deps/lightning/lib/jit_names.c"
            "deps/lightning/lib/jit_note.c"
            "deps/lightning/lib/jit_print.c"
            "deps/lightning/lib/jit_size.c"
            "deps/lightning/lib/lightning.c"
        )
    endif()
    list(APPEND COMPONENT_SRCS
        "frontend/pcsxr-threads.c"
        "plugins/gpulib/gpu_async.c"
    )
endif()

set(COMPONENT_ADD_INCLUDEDIRS
    "."
    "include"
    "include/lightning"
    "libpcsxcore"
    "frontend"
    "deps/libchdr/include"
    "deps/libchdr/deps/zlib-1.3.1"
    "deps/libchdr/deps/lzma-24.05/include"
    "deps/libchdr/deps/zstd-1.5.6/lib"
    "deps/libchdr/deps/zstd-1.5.6/lib/common"
    "deps/libchdr/deps/zstd-1.5.6/lib/decompress"
    "plugins"
    "plugins/gpulib"
    "plugins/gpu_unai"
)

if(CONFIG_IDF_TARGET_ESP32P4)
    list(APPEND COMPONENT_ADD_INCLUDEDIRS
        "deps/lightning/include"
        "include/lightrec"
        "deps/lightrec"
    )
endif()

set(COMPONENT_REQUIRES "retro-go" "esp_mm" "pthread")

register_component()

# libpcsxcore's plugin API uses names such as SPUinit as function-pointer
# typedefs.  Build the statically linked dfsound entry points under the same
# builtin_SPU* namespace previously used by spunull so those typedefs and the
# concrete functions never collide in a translation unit.
set(DFSOUND_ENTRYPOINT_DEFINES
    "P_HAVE_PTHREAD=1"
    "SPUinit=builtin_SPUinit"
    "SPUshutdown=builtin_SPUshutdown"
    "SPUopen=builtin_SPUopen"
    "SPUclose=builtin_SPUclose"
    "SPUwriteRegister=builtin_SPUwriteRegister"
    "SPUreadRegister=builtin_SPUreadRegister"
    "SPUwriteDMAMem=builtin_SPUwriteDMAMem"
    "SPUreadDMAMem=builtin_SPUreadDMAMem"
    "SPUplayADPCMchannel=builtin_SPUplayADPCMchannel"
    "SPUfreeze=builtin_SPUfreeze"
    "SPUregisterCallback=builtin_SPUregisterCallback"
    "SPUregisterScheduleCb=builtin_SPUregisterScheduleCb"
    "SPUasync=builtin_SPUasync"
    "SPUplayCDDAchannel=builtin_SPUplayCDDAchannel"
    "SPUsetCDvol=builtin_SPUsetCDvol"
)
set_source_files_properties(
    "plugins/dfsound/dma.c"
    "plugins/dfsound/freeze.c"
    "plugins/dfsound/registers.c"
    "plugins/dfsound/spu.c"
    PROPERTIES COMPILE_DEFINITIONS "${DFSOUND_ENTRYPOINT_DEFINES}"
)

if(CONFIG_IDF_TARGET_ESP32P4 AND PCSX_NDRC_RV32_SELFTEST AND NOT PCSX_NDRC_RV32_FULL_CORE AND NOT PCSX_DUAL_DYNAREC)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE PCSX_NDRC_RV32_SELFTEST=1)
    if(PCSX_NDRC_RV32_LIVE_CANARY)
        target_compile_definitions(${COMPONENT_LIB} PRIVATE PCSX_NDRC_RV32_LIVE_CANARY=1)
    endif()
endif()
if(CONFIG_IDF_TARGET_ESP32P4 AND PCSX_DUAL_DYNAREC AND NOT PCSX_NDRC_RV32_FULL_CORE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE PCSX_DUAL_DYNAREC=1)
endif()
if(CONFIG_IDF_TARGET_ESP32P4 AND PCSX_NDRC_RV32_FULL_CORE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        PCSX_NDRC_RV32_FULL_CORE=1
    )
    if(PCSX_NDRC_RV32_DIFF)
        target_compile_definitions(${COMPONENT_LIB} PRIVATE DRC_DBG=1)
    endif()
    if(PCSX_NDRC_RV32_PROFILE)
        target_compile_definitions(${COMPONENT_LIB} PRIVATE NDRC_GTE_PROFILE=1)
    endif()
endif()

if(PCSX_PORT_PROFILE OR RG_ENABLE_PROFILING)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE PCSX_PORT_PROFILE=1)
endif()

add_compile_options(-fsigned-char -fdata-sections -ffunction-sections)

if(CONFIG_IDF_TARGET_ESP32P4 AND (NOT PCSX_NDRC_RV32_FULL_CORE OR PCSX_DUAL_DYNAREC))
    add_definitions(
        -DNDEBUG
        -DLIGHTREC
        -DLIGHTREC_STATIC
        -DLIGHTREC_CUSTOM_MAP=0
        -DLIGHTREC_CODE_INV=0
        -DLIGHTREC_ENABLE_THREADED_COMPILER=0
        -DLIGHTREC_ENABLE_DISASSEMBLER=0
        -DLIGHTREC_NO_DEBUG=1
        -DUSE_ASYNC_GPU=1
        -D__WORDSIZE=32
        -D__riscv_xlen=32
    )
elseif(CONFIG_IDF_TARGET_ESP32P4)
    add_definitions(
        -DNDEBUG
        -DUSE_ASYNC_GPU=1
        -D__WORDSIZE=32
        -D__riscv_xlen=32
    )
else()
    add_definitions(-DNDEBUG -DDRC_DISABLE)
endif()

# Force SPIRAM for all core data on ESP32-S3
if(CONFIG_IDF_TARGET_ESP32S3)
    set_source_files_properties(
        "libpcsxcore/psxbios.c"
        "libpcsxcore/psxinterpreter.c"
        "libpcsxcore/mdec.c"
        "libpcsxcore/cdrom.c"
        "libpcsxcore/r3000a.c"
        "libpcsxcore/gte.c"
        PROPERTIES COMPILE_FLAGS "-mfix-esp32-psram-cache-issue"
    )
endif()

rg_setup_compile_options(
    -O3
    -ffast-math
    -D_GNU_SOURCE
    -DP_HAVE_MMAP=0
    -DNO_MMAP
    -DNO_PTHREAD
    -DGPULIB_USE_MMAP=1
    -DGPU_UNAI
    -DUSE_GPULIB
    -DHAVE_CHD
    -DZ7_ST
    -DZSTD_DISABLE_ASM
    -DHAVE_RETRO_GO=1
    -DRETRO_GO
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-strict-aliasing
    -Wno-maybe-uninitialized
    $<$<COMPILE_LANGUAGE:C>:-Wno-pointer-sign>
    -Wno-error=format
    -Wno-format
    $<$<COMPILE_LANGUAGE:C>:-Wno-override-init>
)
