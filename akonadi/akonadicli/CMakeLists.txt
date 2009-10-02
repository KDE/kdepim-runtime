
set(akonadi_bin_SRCS
  main.cpp listcommand.cpp fetchcommand.cpp deletecommand.cpp storecommand.cpp appendcommand.cpp
)


kde4_add_executable(akonadi_bin ${akonadi_bin_SRCS})
set_target_properties(akonadi_bin PROPERTIES OUTPUT_NAME akonadi)

target_link_libraries( akonadi_bin ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDE4_KDECORE_LIBS} ${QT_QTCORE_LIBRARY} )

#install(TARGETS akonadi_bin ${INSTALL_TARGETS_DEFAULT_ARGS} )
