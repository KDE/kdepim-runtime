include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
        ${LIBMAPI_INCLUDE_DIRS}
	${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LIBMAPI_DEFINITIONS}" )


########### next target ###############

set( ocresource_SRCS
     ocresource.cpp
     main.cpp
)

install( FILES ocresource.desktop ocresource.svg DESTINATION "${CMAKE_INSTALL_PREFIX}/share/apps/akonadi/agents" )

kde4_automoc(${ocresource_SRCS})

kde4_add_executable(akonadi_oc_resource RUN_UNINSTALLED ${ocresource_SRCS})

target_link_libraries(akonadi_oc_resource akonadi ${QT_QTDBUS_LIBRARIES} ${LIBMAPI_LIBRARIES} ${KDE4_KIO_LIBRARY} kmimeakonadi)

install(TARGETS akonadi_oc_resource DESTINATION ${BIN_INSTALL_DIR})

