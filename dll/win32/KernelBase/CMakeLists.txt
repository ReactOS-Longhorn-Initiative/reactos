
add_definitions(-D_KERNELBASE_)

spec2def(KernelBase.dll KernelBase.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    DllMain.c
    KB.h)

add_library(KernelBase MODULE
    ${SOURCE}
    KernelBase.rc
    ${CMAKE_CURRENT_BINARY_DIR}/KernelBase_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/KernelBase.def)

target_link_libraries(KernelBase)
set_module_type(KernelBase win32dll ENTRYPOINT DllMain)
add_importlibs(KernelBase ntdll ntdll_vista kernel32_vista advapi32 kernel32)
add_dependencies(KernelBase psdk)
add_cd_file(TARGET KernelBase DESTINATION reactos/system32 FOR all)