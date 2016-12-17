include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )


add_subdirectory(calendar)
add_subdirectory(contacts)

