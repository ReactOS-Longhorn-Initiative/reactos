
#add_subdirectory(bdaplgin) #disabled in trunk
if(ARCH STREQUAL "i386")
    if (NOT MSDX)
        add_subdirectory(d3d8) #disabled in trunk, see wine subfolder
    endif()
endif()
if (NOT MSDX)
    add_subdirectory(d3d8thk)
endif()
if (NOT MSDX OR DXTEST)
    add_subdirectory(d3d9)
endif()
if(ARCH STREQUAL "i386")
    if (NOT MSDX)
        add_subdirectory(d3dim) #disabled in trunk, see wine subfolder
        add_subdirectory(d3drm)
    endif()
endif()
if (NOT MSDX)
    add_subdirectory(d3dxof)
    add_subdirectory(ddraw) #disabled in trunk, see wine subfolder
endif()
add_subdirectory(dinput)
add_subdirectory(dinput8)
if(ARCH STREQUAL "i386")
    add_subdirectory(dmusic)
    add_subdirectory(dplayx)
endif()
add_subdirectory(dsound_new) #disabled in trunk
#add_subdirectory(ksproxy) Not needed at the moment
add_subdirectory(ksuser)
#add_subdirectory(msdvbnp) #disabled in trunk
#add_subdirectory(msvidctl) #disabled in trunk
add_subdirectory(wine)