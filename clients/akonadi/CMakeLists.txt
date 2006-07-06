include_directories( ${CMAKE_SOURCE_DIR}/libkmime )

set(akonadi_bin_SRCS 
  main.cpp listcommand.cpp
)

kde4_automoc(${akonadi_SRCS})

kde4_add_executable(akonadi_bin ${akonadi_bin_SRCS})
set_target_properties(akonadi_bin PROPERTIES OUTPUT_NAME akonadi)

target_link_libraries( akonadi_bin akonadi
  ${QT_QTCORE_LIBRARIES} )

install(TARGETS akonadi_bin DESTINATION ${BIN_INSTALL_DIR} )
