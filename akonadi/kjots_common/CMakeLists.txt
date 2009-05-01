project(kjots_common)

set( kjots_common_SRCS
    kjotsmodel.cpp
    kjotspage.cpp
    kjotsakonadipage.cpp
    kjotswidget.cpp
)

# Temporary for development
FILE(WRITE ${PROJECT_BINARY_DIR}/datadir.h "#define DATADIR \"" ${PROJECT_SOURCE_DIR}/data/ "\"\n\n" )
include_directories( ${PROJECT_BINARY_DIR} )

kde4_add_library(kjots_common SHARED ${kjots_common_SRCS} )

target_link_libraries(kjots_common
                      akonadi_next
                      grantlee_core_library
                      ${KDE4_AKONADI_LIBS}
                      ${KDE4_KIO_LIBS}
)

set_target_properties(kjots_common PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )

install(TARGETS kjots_common ${INSTALL_TARGETS_DEFAULT_ARGS})

# TODO Figure out how to exclude the .svn dirs.
install(DIRECTORY themes DESTINATION ${DATA_INSTALL_DIR}/kjotsrewrite PATTERN *.svn EXCLUDE )

