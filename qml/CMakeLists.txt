add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

add_subdirectory( kde )
add_subdirectory( akonadi )
