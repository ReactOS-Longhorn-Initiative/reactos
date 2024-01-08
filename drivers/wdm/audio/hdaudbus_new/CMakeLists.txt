
remove_definitions(-D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    adsp.cpp
    buspdo.cpp
    fdo.cpp
    hdac_controller.cpp
    hdac_stream.cpp
    hdaudio.cpp
    nhlt.cpp
    sgpc.cpp
    sklhdaudbus.cpp
    sof-tplg.cpp)

add_library(hdaudbus MODULE ${SOURCE} sklhdaudbus.rc)
set_module_type(hdaudbus kernelmodedriver)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(hdaudbus PRIVATE -Wno-sign-compare -Wno-switch -Wno-unknown-pragmas -Wno-unused-but-set-variable -Wno-unused-variable -Wno-write-strings)
endif()

target_link_libraries(hdaudbus wdf01000 ntoskrnl_vista libcntpr ${PSEH_LIB})
add_importlibs(hdaudbus ntoskrnl hal)
add_cd_file(TARGET hdaudbus DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(hdaudbus hdaudbus.inf)
