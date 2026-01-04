list(APPEND SOURCE
    main.c
    profiler.c
    test_win32_tga.c
    test_ddraw_tga.c
    tga.c
    fps.h
    test_d3d7.c
    test_d3d7_textured.c
    test_d3d7_textured_fs.c
    test_d3d8.c
    test_d3d9.c
    test_d3d9_textured.c
    test_d3d9_textured_fs.c
    test_gl11.c
    test_gl11_textured.c
    test_gl11_textured_fs.c
    test_gl20.c
    test_gl20_textured.c
    test_gl42.c
    test_gl42_textured.c
    test_gl42_textured_fs.c
    profiler.h
    tga.h
    resource.h)

add_rc_deps(w32prof.rc ${CMAKE_CURRENT_SOURCE_DIR}/logo_test.tga)

add_executable(w32prof ${SOURCE} w32prof.rc)
set_module_type(w32prof win32gui UNICODE)
add_importlibs(w32prof user32 gdi32 advapi32 opengl32 ddraw d3d8 d3d9 msvcrt kernel32)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(w32prof PRIVATE -Wno-error)
endif()

if(MSVC)
    add_importlibs(w32prof ntdll)
endif()

add_cd_file(TARGET w32prof DESTINATION reactos/system32 FOR all)
