set(COMPONENT_SRCS
    "audio.cpp" "basetimer.cpp" "bitwisemath.cpp" "cpu.cpp"
    "flash.cpp" "flashfile.cpp" "interrupts.cpp" "ram.cpp" "rom.cpp"
    "serial.cpp" "t0.cpp" "t1.cpp" "video.cpp" "vmu.cpp" "native_io.cpp")
set(COMPONENT_ADD_INCLUDEDIRS "." "libretro-common/include")
register_component()
rg_setup_compile_options(-O2)
