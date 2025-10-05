

list(APPEND SOURCE
    src/F3DAudio.c
    src/FACT.c
    src/FACT3D.c
    src/FACT_internal.c
    src/FAPOBase.c
    src/FAPOFX.c
    src/FAPOFX_echo.c
    src/FAPOFX_eq.c
    src/FAPOFX_masteringlimiter.c
    src/FAPOFX_reverb.c
    src/FAudio.c
    src/FAudioFX_reverb.c
    src/FAudioFX_volumemeter.c
    src/FAudio_internal.c
    src/FAudio_internal_simd.c
    src/FAudio_operationset.c
    src/FAudio_platform_win32.c
    src/matrix_defaults.inl)

add_library(faudio STATIC ${SOURCE})
target_compile_definitions(faudio PUBLIC FAUDIO_WIN32_PLATFORM HAVE_WMADEC NDEBUG)
target_include_directories(faudio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_dependencies(faudio psdk)
