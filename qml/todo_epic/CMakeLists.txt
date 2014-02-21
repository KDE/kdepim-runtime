project(TODOTESTAPPLET)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

set(todo_SRCS
    todo.cpp
    todomodel.cpp
    todofiltermodel.cpp
    todotreeview.cpp
    todoitemdelegate.cpp
)
qt4_add_dbus_interface(todo_SRCS org.kde.korganizer.Korganizer.xml korganizer_interface)
qt4_add_dbus_interface(todo_SRCS org.kde.Korganizer.Calendar.xml calendar_interface)
kde4_add_plugin(todo_test ${todo_SRCS})
set(MY_LIBRARIES
    ${KDE4_PLASMA_LIBS}
    ${KDE4_AKONADI_LIBS}
    ${KDE4_KCALCORE_LIBS}
    ${KDE4_KCALUTILS_LIBS}
    ${KDE4_KABC_LIBS}
    ${KDEPIMLIBS_AKONADI_KCAL_LIBS})
if (${KDE_VERSION} VERSION_LESS 4.5.0)
set(MY_LIBRARIES
    ${MY_LIBRARIES}
    ${KDE4_KIO_LIBS})
endif(${KDE_VERSION} VERSION_LESS 4.5.0)
target_link_libraries(todo_test ${MY_LIBRARIES})
install(TARGETS todo_test DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES metadata.desktop DESTINATION ${SERVICES_INSTALL_DIR})
