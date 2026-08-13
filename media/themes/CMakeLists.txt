
# Blackshade is just a placeholder for now.
# add_subdirectory(Blackshade)
# Aero is a v4 (Vista-format) style and is compiled by a host tool rather than
# linked from a .rc - see Aero/CMakeLists.txt.  It needs that tool, which only
# builds on a Windows host.
if(CMAKE_HOST_WIN32)
    add_subdirectory(Aero)
endif()
add_subdirectory(Lautus)
add_subdirectory(Lunar)
add_subdirectory(Mizu)
add_subdirectory(Modern)
