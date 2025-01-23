add_definitions(-D__WINESRC__ -D_KERNEL32_ -DWINVER=0x601 -D_WIN32_WINNT=0x601)
add_definitions(
    -D__WINESRC__
    -D_WINE
    -D__ROS_LONG64__
    -Dwcsnicmp=_wcsnicmp
    -Dwcsicmp=_wcsicmp
    -Dstrnicmp=_strnicmp
    -Dswprintf=_swprintf
    -D_KERNELBASE_)

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601 -DWINVER=0x601)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE_STATIC
    main.c
    path.c)

add_library(kernelbase_static ${SOURCE_STATIC})
target_link_libraries(kernelbase_static ${PSEH_LIB})
add_dependencies(kernelbase_static psdk)