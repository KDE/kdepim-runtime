find_library(STRIGIQTDBUSCLIENT_LIBRARY NAMES strigiqtdbusclient
  PATHS
  /usr/lib
  /usr/local/lib
  ${LIB_INSTALL_DIR}
  $ENV{STRIGI_HOME}/lib
)
macro_log_feature(STRIGIQTDBUSCLIENT_LIBRARY "strigi indexing" "Strigi built with indexing" "http://strigi.sourceforge.net/" FALSE "" "For Akonadi tools such as the message searchprovider")


include_directories(
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

if ( STRIGIQTDBUSCLIENT_LIBRARY )

include_directories(${STRIGI_INCLUDE_DIR} ${STRIGI_INCLUDE_DIR}/strigi)

kde4_add_executable(akonadi_strigi_feeder strigifeeder.cpp)
target_link_libraries(akonadi_strigi_feeder
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${STRIGIQTDBUSCLIENT_LIBRARY}
  ${KDE4_KIO_LIBS}
)

install(TARGETS akonadi_strigi_feeder ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES strigifeeder.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")

endif ( STRIGIQTDBUSCLIENT_LIBRARY )
