include_directories(
  ${CMAKE_SOURCE_DIR}/akonadi
  ${CMAKE_SOURCE_DIR}/akonadi/libakonadi
  ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( kabcresource_SRCS
  kabcresource.cpp
  main.cpp
)

install( FILES kabcresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )


kde4_add_executable(akonadi_kabc_resource RUN_UNINSTALLED ${kabcresource_SRCS})

target_link_libraries(akonadi_kabc_resource akonadicore ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS} ${KDE4_KIO_LIBRARY})

install(TARGETS akonadi_kabc_resource DESTINATION ${BIN_INSTALL_DIR})
