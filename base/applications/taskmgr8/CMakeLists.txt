
add_executable(taskmgr8 taskmgr8.c taskmgr8.rc)
set_module_type(taskmgr8 win32gui UNICODE)
add_importlibs(taskmgr8 user32 gdi32 comctl32 shell32 shlwapi advapi32 psapi msvcrt kernel32)
add_cd_file(TARGET taskmgr8 DESTINATION reactos/system32 FOR all)
