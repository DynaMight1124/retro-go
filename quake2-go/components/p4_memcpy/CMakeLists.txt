idf_component_register(
	SRCS "p4_memcpy.cpp" "p4_memset.cpp"
	INCLUDE_DIRS "."
)

# mem_cpy_p4.hpp needs C++20 concepts and C++23 [[assume]].
target_compile_options(${COMPONENT_LIB} PRIVATE "-std=gnu++23")