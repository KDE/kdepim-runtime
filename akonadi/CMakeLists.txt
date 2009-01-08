add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

install ( FILES defaultcalendar defaultaddressbook birthdays DESTINATION ${DATA_INSTALL_DIR}/akonadi/firstrun )
