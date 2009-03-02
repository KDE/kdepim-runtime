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

kde4_add_library(kjots_common ${kjots_common_SRCS} )

target_link_libraries(kjots_common
                      akonadi_next
#                       /home/stephen/Development/prefix/lib/libakonadi_next.so
                      ${KDE4_AKONADI_LIBS}
                      ${KDE4_KIO_LIBS}
)

  FILE(WRITE somefile KDEPIMLibs__akonadi-kde)
