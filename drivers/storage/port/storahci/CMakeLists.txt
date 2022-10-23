list(APPEND SOURCE
    common.c
    entrypts.c
    hbastat.c
    io.c
    pnppower.c
    storport_vista.c
    util.c)

add_library(storahci MODULE ${SOURCE} storahci.rc)


remove_definitions(-D_WIN32_WINNT=0x502 -DNTDDI_VERSION)

add_definitions(-D_WIN32_WINNT=0x0A00)
add_definitions(-DNTDDI_VERSION=0x0A000005)
set_entrypoint(storahci DriverEntry)
set_module_type(storahci kernelmodedriver)
add_importlibs(storahci storport ntoskrnl hal)
#add_cd_file(TARGET storahci DESTINATION reactos/system32/drivers NO_CAB FOR all)
#add_driver_inf(storahci storahci.inf)
