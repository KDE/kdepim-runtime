find_library(STRIGIQTDBUSCLIENT_LIBRARY NAMES strigiqtdbusclient
  PATHS
  /usr/lib
  /usr/local/lib
  ${LIB_INSTALL_DIR}
  $ENV{STRIGI_HOME}/lib
)
macro_log_feature(STRIGIQTDBUSCLIENT_LIBRARY "strigi indexing" "Strigi built with indexing" "http://strigi.sourceforge.net/" FALSE "" "For Akonadi tools such as the mail threader agent")


include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

if ( STRIGIQTDBUSCLIENT_LIBRARY )

include_directories( ${STRIGI_INCLUDE_DIR} ${STRIGI_INCLUDE_DIR}/strigi )

set( mailthreaderagent_SRCS
  mailthreaderagent.cpp
)


kde4_add_executable(akonadi_mailthreader_agent RUN_UNINSTALLED ${mailthreaderagent_SRCS})

target_link_libraries(akonadi_mailthreader_agent ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY}  ${STRIGIQTDBUSCLIENT_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KMIME_LIBS} )

install(TARGETS akonadi_mailthreader_agent ${INSTALL_TARGETS_DEFAULT_ARGS})
install( FILES mailthreaderagent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

endif ( STRIGIQTDBUSCLIENT_LIBRARY )
