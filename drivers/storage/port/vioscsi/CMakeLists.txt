
remove_definitions(-D_WIN32_WINNT=0x502)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/virtio
                    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/hbaapi/)

list(APPEND SOURCE
    virtio_pci.c
    vioscsi.c
    helper.c
    utils.c)

add_library(vioscsi MODULE ${SOURCE} vioscsi.rc)

if(MSVC)
    target_compile_options(vioscsi PRIVATE /wd4189)
endif()

target_compile_definitions(vioscsi PRIVATE
    _WIN32_WINNT=0x602
    NTDDI_VERSION=0x06020000) # NTDDI_WIN8

target_link_libraries(vioscsi virtio hbaapi scsiwmi ntoskrnl_vista libcntpr)
set_module_type(vioscsi kernelmodedriver ENTRYPOINT DriverEntry)
add_importlibs(vioscsi storport ntoskrnl hal)
add_cd_file(TARGET vioscsi DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_driver_inf(vioscsi vioscsi.inf)
