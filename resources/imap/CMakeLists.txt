set(imap_LIB_SRCS imaplib.cpp socket.cpp)

kde4_add_library(imap SHARED ${imap_LIB_SRCS})

target_link_libraries(imap ${QT_AND_KDECORE_LIBS} ${QT_QTSQL_LIBRARY} ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KIMAP_LIBS})

set_target_properties(imap PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS imap DESTINATION ${LIB_INSTALL_DIR})

include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( imaplibresource_SRCS
  setupserver.cpp
  imaplibresource.cpp
  settings.cpp
)

install( FILES imaplibresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kcfg_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imaplib.kcfg org.kde.Akonadi.Imaplib.Settings )
qt4_add_dbus_adaptor( imaplibresource_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imaplib.Settings.xml settings.h Settings
     )
qt4_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imaplibresource.h org.kde.Akonadi.Imaplib.Resource.xml )
qt4_add_dbus_adaptor( imaplibresource_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imaplib.Resource.xml
        imaplibresource.h ImaplibResource::ImaplibResource
     )
kde4_add_kcfg_files(imaplibresource_SRCS settingsbase.kcfgc)
kde4_add_executable(akonadi_imaplib_resource RUN_UNINSTALLED ${imaplibresource_SRCS})

target_link_libraries(akonadi_imaplib_resource ${AKONADI_LIBS} ${QT_QTDBUS_LIBRARIES} imap kmime  ${KDE4_MAILTRANSPORT_LIBS})

install(TARGETS akonadi_imaplib_resource DESTINATION ${BIN_INSTALL_DIR})

