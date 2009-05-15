set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set(queuer_srcs queuer.cpp)
kde4_add_executable(queuer TEST ${queuer_srcs})
target_link_libraries(queuer ${KDE4_KDEUI_LIBS} ${KDE4_MAILTRANSPORT_LIBS} ${KDE4_KMIME_LIBS} outboxinterface)

