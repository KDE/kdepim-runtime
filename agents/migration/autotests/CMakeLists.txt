
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(schedulertest schedulertest.cpp ../migrationscheduler.cpp ../migrationexecutor.cpp)
target_link_libraries(schedulertest
    gidmigration
    KF5::AkonadiCore
    Qt::Test
)
add_test(NAME schedulertest COMMAND schedulertest)
