add_definitions(
    -D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)
include_directories("/include/")

if (MSVC)
    # Disable warning C4090: 'function': different 'const' qualifiers
    # Disable warning C4146: unary minus operator applied to unsigned type, result still unsigned
    add_compile_options(/wd4090 /wd4146)
endif()

add_subdirectory(libs)