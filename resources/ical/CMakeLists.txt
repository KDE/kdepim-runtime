include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${QT_QTDBUS_INCLUDE_DIR}
	${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( icalresource_SRCS
  icalresource.cpp
  main.cpp
)

install( FILES icalresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )


kde4_add_executable(akonadi_ical_resource RUN_UNINSTALLED ${icalresource_SRCS})

target_link_libraries(akonadi_ical_resource akonadi ${QT_QTDBUS_LIBRARY}  ${KDE4_KCAL_LIBS} ${KDE4_KIO_LIBRARY})

install(TARGETS akonadi_ical_resource DESTINATION ${BIN_INSTALL_DIR})

