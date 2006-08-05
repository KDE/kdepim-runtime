include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/resources/include
	${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions" )


########### next target ###############

set( icalresource_SRCS
  icalresource.cpp
  main.cpp
)

install( FILES icalresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/apps/akonadi/agents" )

kde4_automoc( ${icalresource_SRCS} )

kde4_add_executable(akonadi_ical_resource RUN_UNINSTALLED ${icalresource_SRCS})

target_link_libraries(akonadi_ical_resource akonadi akonadiresources ${QT_QTCORE_LIBRARIES} ${QT_QTDBUS_LIBRARIES} ${KDE4_KDECORE_LIBS} ${KDE4_KCAL_LIBS})

install(TARGETS akonadi_ical_resource DESTINATION ${BIN_INSTALL_DIR})






