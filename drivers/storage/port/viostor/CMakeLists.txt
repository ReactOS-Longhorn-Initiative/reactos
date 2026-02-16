
remove_definitions(-D_WIN32_WINNT=0x502)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/virtio)

list(APPEND SOURCE
    virtio_pci.c
    virtio_stor.c
    virtio_stor_hw_helper.c
    virtio_stor_utils.c)

add_library(viostor MODULE ${SOURCE} virtio_stor.rc)

if(MSVC)
    target_compile_options(viostor PRIVATE /wd4189 /wd4267)
endif()

target_compile_definitions(viostor PUBLIC
    _WIN32_WINNT=0x602
    NTDDI_VERSION=0x06020000) # NTDDI_WIN8

target_link_libraries(viostor virtio ntoskrnl_vista libcntpr)
set_module_type(viostor kernelmodedriver ENTRYPOINT DriverEntry)
add_importlibs(viostor storport ntoskrnl hal)
add_cd_file(TARGET viostor DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_driver_inf(viostor viostor.inf)
