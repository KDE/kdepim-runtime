include_directories(
  ${CMAKE_SOURCE_DIR}/akonadi
  ${QT_QTDBUS_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( kcalresource_SRCS
  kcalresource.cpp
)

install( FILES kcalresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )


kde4_add_executable(akonadi_kcal_resource RUN_UNINSTALLED ${kcalresource_SRCS})

target_link_libraries(akonadi_kcal_resource ${KDE4_AKONADI_LIBS} akonadi-kcal ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KCAL_LIBS} ${KDE4_KIO_LIBRARY})

install(TARGETS akonadi_kcal_resource DESTINATION ${BIN_INSTALL_DIR})
