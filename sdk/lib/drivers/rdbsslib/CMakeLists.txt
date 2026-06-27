# The RDBSS framework (rdbss.c and the rx*/fcb DDK headers) implements the NT5.2
# RDBSS API; the NT6.0 RDBSS layout differs substantially (FCB.Specific removed,
# Rx* signatures changed) and is not ported. Build against the NT5.2 RDBSS surface.
remove_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)
add_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)

add_definitions(-DUNICODE -D_UNICODE -DRDBSS_TRACKER)

include_directories(${REACTOS_SOURCE_DIR}/drivers/filesystems/mup
                    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/copysup)

list(APPEND SOURCE
    rdbss.c)

add_library(rdbsslib ${SOURCE})
target_link_libraries(rdbsslib PRIVATE pseh)
add_dependencies(rdbsslib bugcodes xdk)
