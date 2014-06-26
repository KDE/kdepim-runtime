find_package(Sasl2)
set_package_properties(Sasl2 PROPERTIES DESCRIPTION "cyrus-sasl" URL "http://asg.web.cmu.edu/sasl/sasl-library.html" TYPE REQUIRED PURPOSE "Login authentication for Gmail resource")

add_definitions(-D_POSIX_SOURCE)

include_directories(${SASL2_INCLUDE_DIR}
                    ${SASL2_INCLUDE_DIR}/sasl)

set(kdexoauth2sasl_SRCS
    plugin_common.c
    xoauth2plugin.c
    xoauth2plugin_init.c
)

kde4_add_library(kdexoauth2 SHARED ${kdexoauth2sasl_SRCS})
set_target_properties(kdexoauth2 PROPERTIES SOVERSION 3 VERSION 3.0.0)

install(TARGETS kdexoauth2 DESTINATION ${LIB_INSTALL_DIR}/sasl2)
