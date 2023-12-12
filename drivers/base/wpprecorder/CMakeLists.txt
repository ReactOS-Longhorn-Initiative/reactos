
spec2def(wpprecorder.sys wpprecorder.spec ADD_IMPORTLIB)

add_library(wpprecorder MODULE wpprecorder.c wpprecorder.rc ${CMAKE_CURRENT_BINARY_DIR}/wpprecorder.def)
target_link_libraries(wpprecorder ${PSEH_LIB})
set_module_type(wpprecorder kernelmodedriver)
add_importlibs(wpprecorder ntoskrnl hal)
add_cd_file(TARGET wpprecorder DESTINATION reactos/system32/drivers FOR all)
