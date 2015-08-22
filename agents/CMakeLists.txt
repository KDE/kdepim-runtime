project(agents)
cmake_minimum_required(VERSION 2.8.12)

add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

add_subdirectory( maildispatcher )
add_subdirectory( newmailnotifier )
add_subdirectory( migration )
add_subdirectory( invitations )

