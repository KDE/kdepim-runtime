if (NOT CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
   add_definitions(-D_POSIX_SOURCE)
endif()

if (WIN32)
    add_definitions(-DWIN32)
elseif (APPLE)
    add_definitions(-Dmacintosh)
endif()

set(kdexoauth2sasl_SRCS
    xoauth2plugin.c
    xoauth2plugin_init.c
    plugin_common.c
)

add_library(kdexoauth2 SHARED ${kdexoauth2sasl_SRCS})
target_include_directories(kdexoauth2 PRIVATE ${Sasl2_INCLUDE_DIRS})
set_target_properties(kdexoauth2 PROPERTIES
    C_VISIBILITY_PRESET default
    SOVERSION 3
    VERSION 3.0.0)

install(TARGETS kdexoauth2 DESTINATION ${KDE_INSTALL_LIBDIR}/sasl2)
