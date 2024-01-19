
# Blackshade is just a placeholder for now.
# add_subdirectory(Blackshade)
add_subdirectory(Lautus)
add_subdirectory(Lunar)
add_subdirectory(Mizu)
add_subdirectory(Modern)
add_subdirectory(Reactive)

set(_ae_path ${CMAKE_CURRENT_SOURCE_DIR}/Aero)
if(EXISTS ${_ae_path})
    add_subdirectory(${_ae_path} ${CMAKE_CURRENT_BINARY_DIR}/Aero)
endif()
