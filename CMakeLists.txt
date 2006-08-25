configure_file(akonadi-prefix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/akonadi-prefix.h )

add_subdirectory(server)
add_subdirectory(libakonadi)
add_subdirectory(clients)
add_subdirectory(resources)
