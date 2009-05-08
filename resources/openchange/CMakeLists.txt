set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${LIBMAPI_INCLUDE_DIRS}
    ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I%{LIBMAPI_INCLUDE_DIRS} ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( ocresource_SRCS
     profileeditdialog.cpp
     profiledialog.cpp
     ocresource.cpp
     lzfu.cpp
)

install( FILES ocresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_executable(akonadi_oc_resource RUN_UNINSTALLED ${ocresource_SRCS})

target_link_libraries(akonadi_oc_resource ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTDBUS_LIBRARY} ${LIBMAPI_LIBRARIES} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${KDEPIMLIBS_KCAL_LIBS})

install(TARGETS akonadi_oc_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

############ tests ###################

# qtestlib unit tests
kde4_add_unit_test( lzfutest lzfutest.cpp lzfu.cpp )
target_link_libraries( lzfutest ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY} )


