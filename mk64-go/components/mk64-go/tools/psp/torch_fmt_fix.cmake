# Injected into every CMake project of the Torch build via
# -DCMAKE_PROJECT_INCLUDE=tools/psp/torch_fmt_fix.cmake: the bundled spdlog/fmt
# does not compile under recent clang with consteval format checking.
add_compile_definitions(FMT_CONSTEVAL=)
