include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( vcarddirresource_SRCS
  vcarddirresource.cpp
)

install( FILES vcarddirresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_executable(akonadi_vcarddir_resource RUN_UNINSTALLED ${vcarddirresource_SRCS})
target_link_libraries(akonadi_vcarddir_resource ${KDE4_AKONADI_LIBS} ${QT_QTCORE_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS})
install(TARGETS akonadi_vcarddir_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
