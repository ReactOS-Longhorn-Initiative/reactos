
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x601)

add_executable(keyedevt_stress keyedevt_stress.c)
set_module_type(keyedevt_stress win32cui UNICODE)
target_link_libraries(keyedevt_stress ${PSEH_LIB})
add_importlibs(keyedevt_stress msvcrt kernel32)
add_cd_file(TARGET keyedevt_stress DESTINATION reactos/system32 FOR all)
