set(COMPONENT_SRCS
    "source/Audio.cpp"
    "source/cart.cpp"
    "source/emojiconversion.cpp"
    "source/filehelpers.cpp"
    "source/filter.cpp"
    "source/fontdata.cpp"
    "source/graphics.cpp"
    "source/Input.cpp"
    "source/logger.cpp"
    "source/mathhelpers.cpp"
    "source/nibblehelpers.cpp"
    "source/picoluaapi.cpp"
    "source/printHelper.cpp"
    "source/stringToDataHelpers.cpp"
    "source/synth.cpp"
    "source/vm.cpp"

    # Libs (All compiled as CXX to handle fix32 integration)
    "libs/z8lua/lapi.c"
    "libs/z8lua/lauxlib.c"
    "libs/z8lua/lbaselib.c"
    "libs/z8lua/lbitlib.c"
    "libs/z8lua/lcode.c"
    "libs/z8lua/lcorolib.c"
    "libs/z8lua/lctype.c"
    "libs/z8lua/ldblib.c"
    "libs/z8lua/ldebug.c"
    "libs/z8lua/ldo.c"
    "libs/z8lua/ldump.c"
    "libs/z8lua/lfunc.c"
    "libs/z8lua/lgc.c"
    "libs/z8lua/linit.c"
    "libs/z8lua/liolib.c"
    "libs/z8lua/llex.c"
    "libs/z8lua/lmem.c"
    "libs/z8lua/loadlib.c"
    "libs/z8lua/lobject.c"
    "libs/z8lua/lopcodes.c"
    "libs/z8lua/loslib.c"
    "libs/z8lua/lparser.c"
    "libs/z8lua/lstate.c"
    "libs/z8lua/lstring.c"
    "libs/z8lua/lstrlib.c"
    "libs/z8lua/ltable.c"
    "libs/z8lua/ltablib.c"
    "libs/z8lua/ltm.c"
    "libs/z8lua/lundump.c"
    "libs/z8lua/lvm.c"
    "libs/z8lua/lzio.c"
    "libs/z8lua/eris.c"
    "libs/z8lua/lpico8lib.c"
    
    "libs/miniz/miniz.c"
)

set(COMPONENT_ADD_INCLUDEDIRS
    "source"
    "libs"
    "libs/z8lua"
    "libs/miniz"
)

set(COMPONENT_REQUIRES "retro-go")

register_component()

# Set properties AFTER register_component
set(LUA_SRCS
    "libs/z8lua/lapi.c"
    "libs/z8lua/lauxlib.c"
    "libs/z8lua/lbaselib.c"
    "libs/z8lua/lbitlib.c"
    "libs/z8lua/lcode.c"
    "libs/z8lua/lcorolib.c"
    "libs/z8lua/lctype.c"
    "libs/z8lua/ldblib.c"
    "libs/z8lua/ldebug.c"
    "libs/z8lua/ldo.c"
    "libs/z8lua/ldump.c"
    "libs/z8lua/lfunc.c"
    "libs/z8lua/lgc.c"
    "libs/z8lua/linit.c"
    "libs/z8lua/liolib.c"
    "libs/z8lua/llex.c"
    "libs/z8lua/lmem.c"
    "libs/z8lua/loadlib.c"
    "libs/z8lua/lobject.c"
    "libs/z8lua/lopcodes.c"
    "libs/z8lua/loslib.c"
    "libs/z8lua/lparser.c"
    "libs/z8lua/lstate.c"
    "libs/z8lua/lstring.c"
    "libs/z8lua/lstrlib.c"
    "libs/z8lua/ltable.c"
    "libs/z8lua/ltablib.c"
    "libs/z8lua/ltm.c"
    "libs/z8lua/lundump.c"
    "libs/z8lua/lvm.c"
    "libs/z8lua/lzio.c"
    "libs/z8lua/eris.c"
    "libs/z8lua/lpico8lib.c"
)

if(NOT CMAKE_SCRIPT_MODE_FILE)
    set_source_files_properties(${LUA_SRCS} PROPERTIES LANGUAGE CXX)
    set_source_files_properties(${LUA_SRCS} PROPERTIES COMPILE_FLAGS "-fwrapv -fno-strict-overflow -Wno-implicit-fallthrough")
    set_source_files_properties(${COMPONENT_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-unused-variable -Wno-unused-function -Wno-format -fwrapv -fno-strict-overflow -Wno-implicit-fallthrough")

    rg_setup_compile_options()
endif()
