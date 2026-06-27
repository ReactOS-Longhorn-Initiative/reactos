# These fltmgr tests target the NT5.2 fltmgr (NTDDI_WS03SP1); align _WIN32_WINNT/WINVER.
remove_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)
add_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)


include_directories(../../include)

list(APPEND FLTMGR_TEST_DRV_SOURCE
    ../../kmtest_drv/kmtest_fsminifilter.c
    fltmgr_load.c)

add_library(FltMgrLoad_drv MODULE ${FLTMGR_TEST_DRV_SOURCE})
set_module_type(FltMgrLoad_drv kernelmodedriver)
target_link_libraries(FltMgrLoad_drv kmtest_printf ${PSEH_LIB})
add_importlibs(FltMgrLoad_drv fltmgr ntoskrnl hal)
target_compile_definitions(FltMgrLoad_drv PRIVATE KMT_STANDALONE_DRIVER KMT_FILTER_DRIVER NTDDI_VERSION=NTDDI_WS03SP1)
#add_pch(FltMgrLoad_drv ../include/kmt_test.h)
add_rostests_file(TARGET FltMgrLoad_drv)
