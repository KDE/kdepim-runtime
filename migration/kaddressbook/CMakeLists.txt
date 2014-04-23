set( kaddressbookmigrator_SRCS kaddressbookmigrator.cpp )

add_executable( kaddressbookmigrator ${kaddressbookmigrator_SRCS} )

target_link_libraries(kaddressbookmigrator  KF5::Abc)

install(TARGETS kaddressbookmigrator ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS  kaddressbookmigrator.desktop DESTINATION ${AUTOSTART_INSTALL_DIR})
