
function(add_d3dx10_target __version)
    set(module d3dx10_${__version})

    spec2def(${module}.dll ${module}.spec ADD_IMPORTLIB)

    list(APPEND SOURCE
        ../${module}/${module}_main.c)

    list(APPEND PCH_SKIP_SOURCE
        ${CMAKE_CURRENT_BINARY_DIR}/${module}_stubs.c)

    add_library(${module} MODULE
        ${SOURCE}
        ${PCH_SKIP_SOURCE}
        version.rc
        ${CMAKE_CURRENT_BINARY_DIR}/${module}.def)

    set_module_type(${module} win32dll)
    add_dependencies(${module} d3d_idl_headers d3dx10_43)
    add_importlibs(${module} d3dx10_43 msvcrt ntdll)
    if(${__version} LESS 39)
        add_importlibs(${module} d3dx10_39)
        if(${__version} LESS 37)
           add_importlibs(${module} d3dx10_37)
        endif()
    endif()
    target_link_libraries(${module} dxguid wine oldnames)
  #  add_pch(${module} ../${module}/precomp.h "${PCH_SKIP_SOURCE}")
    add_cd_file(TARGET ${module} DESTINATION reactos/system32 FOR all)
endfunction()
