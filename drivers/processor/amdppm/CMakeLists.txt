
list(APPEND SOURCE
    amdppm.c
    acpi.c
    cpuid.c
    perfhand.c
    amdppm.h)

add_library(amdppm MODULE ${SOURCE} amdppm.rc)
set_module_type(amdppm kmdfdriver)
target_link_libraries(amdppm ntoskrnl_vista libcntpr ${PSEH_LIB})
add_importlibs(amdppm wdfldr ntoskrnl hal)
add_pch(amdppm amdppm.h SOURCE)
add_cd_file(TARGET amdppm DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(amdppm amdppm.inf)
