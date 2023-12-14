
include_directories(${CMAKE_SOURCE_DIR}/sdk/lib/drivers/wdf/kmdf/inc/private)
list(APPEND SOURCE
      usbhub3.cpp
      ioctl.cpp
      todo.cpp
      hubfdo.cpp)


add_library(usbhub3 MODULE
    ${SOURCE}
    usbhub3.rc)

target_compile_definitions(usbhub3 PRIVATE DEBUG_USE_KDPRINT
    WDF_DEVICE_NO_WDMSEC_H _WIN32_WINNT=0x602 NTDDI_VERSION=0x06020000)

set_module_type(usbhub3 kmdfdriver)
add_importlibs(usbhub3 wdfldr hal ntoskrnl)
#add_pch(usbhub3 usbhub3.h SOURCE)
add_dependencies(usbhub3 bugcodes asm xdk)
add_cd_file(TARGET usbhub3 DESTINATION reactos/system32/drivers NO_CAB FOR all)
