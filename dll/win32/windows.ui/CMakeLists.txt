
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(windows.ui.dll windows.ui.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    accessibilitysettings.c
    inputpane.c
    main.c
    uisettings.c
    uiviewsettings.c
    weakref.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/windows.ui_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/windows.ui.def)
	
add_library(windows.ui SHARED ${SOURCE})
set_module_type(windows.ui win32dll ENTRYPOINT 0)
target_link_libraries(windows.ui wine wine_dll_register uuid)
add_importlibs(windows.ui shell32 shlwapi oleaut32 ole32 combase user32 user32_vista gdi32 ucrtbase advapi32 advapi32_vista kernel32 ntdll )
add_cd_file(TARGET windows.ui DESTINATION reactos/system32 FOR all)
