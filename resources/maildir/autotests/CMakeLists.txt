include(ECMMarkAsTest)

find_package(Qt5Test CONFIG REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
set(KDEPIMLIBS_RUN_ISOLATED_TESTS TRUE)
set(KDEPIMLIBS_RUN_SQLITE_ISOLATED_TESTS TRUE)

add_akonadi_isolated_test( synctest.cpp )
akonadi_add_resourcetest( maildir maildir.js )

