configure_file(config-prefix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-prefix.h )

add_subdirectory(server)
add_subdirectory(libakonadi)
add_subdirectory(clients)
add_subdirectory(resources)
