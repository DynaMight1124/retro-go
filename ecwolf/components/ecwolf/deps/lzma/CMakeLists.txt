cmake_minimum_required( VERSION 3.5.0 )

if(POLICY CMP0054)
	cmake_policy(SET CMP0054 NEW)
endif()

#make_release_only()

add_library(lzma STATIC
	C/7zArcIn.c
	C/7zBuf.c
	C/7zCrc.c
	C/7zCrcOpt.c
	C/7zDec.c
	C/7zStream.c
	C/Bcj2.c
	C/Bra.c
	C/Bra86.c
	C/BraIA64.c
	C/CpuArch.c
	C/Delta.c
	C/LzFind.c
	C/Lzma2Dec.c
	C/LzmaDec.c
	C/LzmaEnc.c
	C/Ppmd7.c
	C/Ppmd7Dec.c
)

target_include_directories(lzma PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/C")

if( "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" )
	target_compile_options(lzma PRIVATE -Wall -Wextra -fomit-frame-pointer)
endif()

target_compile_definitions(lzma PRIVATE -D_7ZIP_PPMD_SUPPPORT)

if( WIN32 )
	target_sources(lzma PRIVATE C/LzFindMt.c C/Threads.c)
else()
	target_compile_definitions(lzma PRIVATE -D_7ZIP_ST)
endif()
