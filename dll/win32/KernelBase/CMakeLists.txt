
add_definitions(
    -D__WINESRC__
    -D_WINE
    -D__ROS_LONG64__
    -D_KERNELBASE_)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601 -DWINVER=0x601)

add_subdirectory(wine)

spec2def(KernelBase.dll KernelBase.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    DllMain.c)

add_library(KernelBase MODULE
    ${SOURCE}
    wine/KernelBase.rc
    ${CMAKE_CURRENT_BINARY_DIR}/KernelBase_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/KernelBase.def)

target_link_libraries(KernelBase kernelbase_static)
set_module_type(KernelBase win32dll ENTRYPOINT DllMain)
add_importlibs(KernelBase ntdll ntdll_vista)
add_dependencies(KernelBase psdk)
add_cd_file(TARGET KernelBase DESTINATION reactos/system32 FOR all)