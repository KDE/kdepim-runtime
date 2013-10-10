
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kde4_add_executable(schedulertest schedulertest.cpp ../migrationscheduler.cpp ../migrationexecutor.cpp)
target_link_libraries(schedulertest
    gidmigration
    ${KDE4_KDEUI_LIBRARY}
    ${KDE4_KDECORE_LIBS}
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTTEST_LIBRARY}
)
add_test(schedulertest schedulertest)