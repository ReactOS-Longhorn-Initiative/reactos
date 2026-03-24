
add_executable(taskmgr8 taskmgr8.c taskmgr8_helpers.c taskmgr8_theme.c taskmgr8_cpu_stats.c taskmgr8_mem_stats.c
    taskmgr8_listutil.c taskmgr8_details.c taskmgr8_services.c taskmgr8.rc)
set_module_type(taskmgr8 win32gui UNICODE)
add_delay_importlibs(taskmgr8 dwmapi)
add_importlibs(taskmgr8 user32 gdi32 comctl32 shell32 shlwapi advapi32 psapi iphlpapi msvcrt kernel32)
add_cd_file(TARGET taskmgr8 DESTINATION reactos/system32 FOR all)
