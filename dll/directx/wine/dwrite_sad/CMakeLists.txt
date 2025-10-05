	
add_definitions(-D__WINESRC__
				-Dwcslwr=_wcslwr
				-Dwcsicmp=_wcsicmp
				-Dwcsdup=_wcsdup
				-Dwcsnicmp=_wcsnicmp)
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/freetype/include)
spec2def(dwrite.dll dwrite.spec ADD_IMPORTLIB)

list(APPEND SOURCE
	#analyzer.c
	#bidi.c
	#bracket.c
	#direction.c
	#font.c
	#freetype.c
	#gdiinterop.c
	#layout.c
	#linebreak.c
	main.c
	#mirror.c
	#opentype.c
	#scripts.c
	#shape.c
	#shapers/arabic.c
	#shapers/arabic_table.c
	dwrite_private.h
	#scripts.h
   # version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dwrite_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dwrite.def)

add_library(dwrite SHARED ${SOURCE})
add_dependencies(dwrite wineheaders d3d_idl_headers)
set_module_type(dwrite win32dll)
target_link_libraries(dwrite wine uuid dxguid)
add_importlibs(dwrite advapi32 msvcrt user32 gdi32 kernel32 ntdll)
#add_cd_file(TARGET dwrite DESTINATION reactos/system32 FOR all)