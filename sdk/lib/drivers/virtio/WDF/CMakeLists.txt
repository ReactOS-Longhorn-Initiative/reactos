
include_directories(BEFORE ../ ${REACTOS_SOURCE_DIR}/sdk/include/wdf/kmdf/1.17)

list(APPEND SOURCE
    VirtIOWdf.c
    PCI.c
    MemPortIO.c
    Dma.c
    Callbacks.c)

add_library(virtio-wdf ${SOURCE})
add_dependencies(virtio-wdf bugcodes xdk)

if(MSVC)
    target_compile_options(virtio-wdf PRIVATE /wd4189)
endif()

target_compile_definitions(virtio-wdf PRIVATE
    _WIN32_WINNT=0x602
    NTDDI_VERSION=0x06020000  # NTDDI_WIN8
    WDF_DEVICE_NO_WDMSEC_H)
