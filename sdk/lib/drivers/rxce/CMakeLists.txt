# The RXCE/RDBSS framework implements the NT5.2 RDBSS API; the NT6.0 RDBSS layout
# differs substantially and is not ported. Build against the NT5.2 RDBSS surface.
remove_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)
add_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)

add_definitions(-DUNICODE -D_UNICODE -DRDBSS_TRACKER)

include_directories(${REACTOS_SOURCE_DIR}/drivers/filesystems/mup)

list(APPEND SOURCE
    rxce.c)

add_library(rxce ${SOURCE})
target_link_libraries(rxce PRIVATE pseh)
add_dependencies(rxce bugcodes xdk)
