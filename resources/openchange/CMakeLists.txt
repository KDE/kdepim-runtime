include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
        ${LIBMAPI_INCLUDE_DIRS}
	${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LIBMAPI_DEFINITIONS}" )


########### next target ###############

set( ocresource_SRCS
     profileeditdialog.cpp
     profiledialog.cpp
     ocresource.cpp
     main.cpp
     lzfu.cpp
)

install( FILES ocresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_executable(akonadi_oc_resource RUN_UNINSTALLED ${ocresource_SRCS})

target_link_libraries(akonadi_oc_resource akonadi ${QT_QTDBUS_LIBRARIES} ${LIBMAPI_LIBRARIES} ${KDE4_KIO_LIBRARY} kmimeakonadi)

install(TARGETS akonadi_oc_resource DESTINATION ${BIN_INSTALL_DIR})

############ tests ###################

# qtestlib unit tests
kde4_add_unit_test( lzfutest lzfutest.cpp lzfu.cpp )
target_link_libraries( lzfutest ${KDE4_KIO_LIBRARY} ${QT_QTTEST_LIBRARY} )


