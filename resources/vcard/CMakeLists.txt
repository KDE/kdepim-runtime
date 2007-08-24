include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( vcardresource_SRCS
  vcardresource.cpp
  main.cpp
)

install( FILES vcardresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )


kde4_add_executable(akonadi_vcard_resource RUN_UNINSTALLED ${vcardresource_SRCS})

target_link_libraries(akonadi_vcard_resource akonadi ${QT_QTCORE_LIBRARIES} ${QT_QTDBUS_LIBRARIES} ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS} ${KDE4_KIO_LIBRARY})

install(TARGETS akonadi_vcard_resource DESTINATION ${BIN_INSTALL_DIR})
