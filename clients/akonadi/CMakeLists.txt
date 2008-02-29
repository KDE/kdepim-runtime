include_directories( ${CMAKE_SOURCE_DIR}/akonadi)

set(akonadi_bin_SRCS
  main.cpp listcommand.cpp fetchcommand.cpp deletecommand.cpp storecommand.cpp appendcommand.cpp
)


kde4_add_executable(akonadi_bin ${akonadi_bin_SRCS})
set_target_properties(akonadi_bin PROPERTIES OUTPUT_NAME akonadi)

target_link_libraries( akonadi_bin akonadicore
  ${QT_QTCORE_LIBRARY} )

install(TARGETS akonadi_bin DESTINATION ${BIN_INSTALL_DIR} )
