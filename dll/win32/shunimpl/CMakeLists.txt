
spec2def(shunimpl.dll shunimpl.spec ADD_IMPORTLIB)

add_library(shunimpl MODULE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/shunimpl.def
)

set_module_type(shunimpl win32dll UNICODE ENTRYPOINT DllMain 12)
add_dependencies(shunimpl xdk)
add_cd_file(TARGET shunimpl DESTINATION reactos/system32 FOR all)
