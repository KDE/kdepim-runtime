add_definitions(-D_POSIX_SOURCE)

set(kdexoauth2sasl_SRCS
    plugin_common.c
    xoauth2plugin.c
    xoauth2plugin_init.c
)

add_library(kdexoauth2 SHARED ${kdexoauth2sasl_SRCS})
target_link_libraries(kdexoauth2 Sasl2::Sasl2)
set_target_properties(kdexoauth2 PROPERTIES SOVERSION 4 VERSION 4.0.0)

install(TARGETS kdexoauth2 DESTINATION ${KDE_INSTALL_LIBDIR}/sasl2)
