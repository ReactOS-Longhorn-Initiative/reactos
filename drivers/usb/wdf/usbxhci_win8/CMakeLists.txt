

list(APPEND XHCI_LIB
    xhcilib/xhci.cpp)

list(APPEND SOURCE
    usbxhci.cpp)

add_library(usbxhci MODULE
    ${SOURCE}
    ${XHCI_LIB}
    usbxhci.rc)

target_compile_definitions(cdrom PRIVATE DEBUG_USE_KDPRINT 
    WDF_DEVICE_NO_WDMSEC_H _WIN32_WINNT=0x603 NTDDI_VERSION=0x06030000)

set_module_type(usbxhci kmdfdriver)
add_importlibs(usbxhci wdfldr hal ntoskrnl) # Matches win8 RTM
add_pch(usbxhci usbxhci.h SOURCE)
#add_cd_file(TARGET usbxhci DESTINATION reactos/system32/drivers NO_CAB FOR all)
