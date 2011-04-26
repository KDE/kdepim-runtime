
find_package (KdepimLibs )
add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS} )
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include_directories ( ${KDE4_INCLUDES} ${KDEPIMLIBS_INCLUDE_DIRS} ) 

set(ispdb_srcs
  main.cpp
  ispdb.cpp
)

kde4_add_executable(ispdb ${ispdb_srcs})

target_link_libraries(ispdb
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}

)

install(TARGETS ispdb ${INSTALL_TARGETS_DEFAULT_ARGS})


add_subdirectory(dataengine)
