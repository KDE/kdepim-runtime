set( kaddressbookmigrator_SRCS kaddressbookmigrator.cpp )

kde4_add_executable( kaddressbookmigrator ${kaddressbookmigrator_SRCS} )

target_link_libraries(kaddressbookmigrator ${KDE4_KDECORE_LIBS} ${KDEPIMLIBS_KABC_LIBS})

install(TARGETS kaddressbookmigrator ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS  kaddressbookmigrator.desktop DESTINATION ${AUTOSTART_INSTALL_DIR})
