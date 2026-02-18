
add_subdirectory(hidpassthrough)

remove_definitions(-D_WIN32_WINNT=0x502)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/virtio
                    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/virtio/WDF)

list(APPEND SOURCE
    utils.c
    KeyMap.c
    IsrDpc.c
    HidTablet.c
    HidMouse.c
    HidKeyboard.c
    HidJoystick.c
    HidConsumer.c
    Hid.c
    Driver.c
    Device.c
    Array.c)

add_library(vioinput MODULE ${SOURCE} vioinput.rc)

if(MSVC)
    target_compile_options(vioinput PRIVATE /wd4101 /wd4189 /wd4267)
endif()

target_compile_definitions(vioinput PRIVATE
    _WIN32_WINNT=0x602
    NTDDI_VERSION=0x06020000  # NTDDI_WIN8
    WDF_DEVICE_NO_WDMSEC_H)

target_link_libraries(vioinput virtio virtio-wdf ntoskrnl_vista libcntpr)
set_module_type(vioinput kmdfdriver)
add_importlibs(vioinput ntoskrnl hal)
add_cd_file(TARGET vioinput DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_driver_inf(vioinput vioinput.inf)
