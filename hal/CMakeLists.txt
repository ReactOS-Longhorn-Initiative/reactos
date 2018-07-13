
project(HAL)

if(MSVC)
    #replace_compile_flags("/Od /Ob0" "/Ox /Ob2 /Ot /Oy /GT /GF")
endif()

if((ARCH STREQUAL "i386") OR (ARCH STREQUAL "amd64"))
    add_subdirectory(halx86)
elseif(ARCH STREQUAL "arm")
    add_subdirectory(halarm)
endif()
