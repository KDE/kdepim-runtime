add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

set( firstrun_srcs
  firstrun.cpp
  main.cpp
)

kde4_add_executable( akonadi-firstrun ${firstrun_srcs} )
target_link_libraries( akonadi-firstrun ${KDE4_AKONADI_LIBS} )

install ( FILES defaultcalendar defaultaddressbook birthdays DESTINATION ${DATA_INSTALL_DIR}/akonadi/firstrun )
