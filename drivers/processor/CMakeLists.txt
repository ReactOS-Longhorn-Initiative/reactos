
add_subdirectory(processr)

if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64")
    add_subdirectory(amdppm)
endif()