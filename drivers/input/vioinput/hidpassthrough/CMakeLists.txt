
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/virtio)

add_library(viohidkmdf MODULE hidpassthrough.c hidpassthrough.rc)
set_module_type(viohidkmdf kernelmodedriver)
add_importlibs(viohidkmdf ntoskrnl hidclass)
add_cd_file(TARGET viohidkmdf DESTINATION reactos/system32/drivers NO_CAB FOR all)
