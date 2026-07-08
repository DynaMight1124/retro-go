set(COMPONENT_SRCS 
    "main.c"
)
set(COMPONENT_ADD_INCLUDEDIRS ".")

idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
    PRIV_INCLUDE_DIRS 
        "../components/sm64-funkey/include"
        "../components/sm64-funkey/src"
        "../components/sm64-funkey/src/pc"
        "../components/sm64-funkey/src/buffers"
    REQUIRES 
        "retro-go"
        "sm64-funkey"
)

# We define these here to ensure main.c compiles correctly
target_compile_definitions(${COMPONENT_LIB} PRIVATE 
    VERSION_US
    ENABLE_SOFTRAST=1
    CONVERT
    NON_MATCHING=1
    AVOID_UB=1
    RETRO_GO=1
    NO_SEGMENTED_MEMORY=1
)
