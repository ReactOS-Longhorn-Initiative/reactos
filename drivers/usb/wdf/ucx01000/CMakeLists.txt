

include_directories(${CMAKE_SOURCE_DIR}/sdk/lib/drivers/wdf/kmdf/inc/private)
list(APPEND SOURCE
    UCX01000.cpp)

add_library(UCX01000 MODULE
    ${SOURCE}
    UCX01000.rc)

target_compile_definitions(UCX01000 PRIVATE DEBUG_USE_KDPRINT
    WDF_DEVICE_NO_WDMSEC_H _WIN32_WINNT=0x603 NTDDI_VERSION=0x06030000)

set_module_type(UCX01000 kmdfdriver)
add_importlibs(UCX01000 wdfldr hal ntoskrnl)
add_pch(UCX01000 UCX01000.h SOURCE)
add_dependencies(UCX01000 bugcodes asm xdk)
add_cd_file(TARGET UCX01000 DESTINATION reactos/system32/drivers NO_CAB FOR all)
