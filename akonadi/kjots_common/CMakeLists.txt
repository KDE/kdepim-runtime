project(kjots_common)

set( kjots_common_SRCS
    kjotspage.cpp
    kjotsakonadipage.cpp
    kjotswidget.cpp
    kjotswidgetpimstyle.cpp
)

# Temporary for development
FILE(WRITE ${PROJECT_BINARY_DIR}/datadir.h "#define DATADIR \"" ${PROJECT_SOURCE_DIR}/data/ "\"\n\n" )
include_directories( ${PROJECT_BINARY_DIR} )

kde4_add_library(kjots_common SHARED ${kjots_common_SRCS} )

target_link_libraries(kjots_common
                      akonadi_next
                      ${KDE4_AKONADI_LIBS}
                      ${KDE4_KIO_LIBS}
)

set_target_properties(kjots_common PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )

install(TARGETS kjots_common ${INSTALL_TARGETS_DEFAULT_ARGS})

  FILE(WRITE somefile KDEPIMLibs__akonadi-kde)
