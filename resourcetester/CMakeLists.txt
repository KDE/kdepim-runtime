project (resourcetester)

find_package(KDE4 REQUIRED)
find_package (KdepimLibs REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)
include(CheckIncludeFiles)

include_directories(
    ${AKONADI_INCLUDE_DIR}
    ${KDE4_INCLUDES}
    ${KDEPIMLIBS_INCLUDE_DIR}
)


set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set( resourcetester_SRCS
  test.cpp
  testbase.cpp
  main.cpp
)

kde4_add_executable(resourcetester RUN_UNINSTALLED ${resourcetester_SRCS})

target_link_libraries( resourcetester
  ${KDE4_AKONADI_LIBS}
  ${QT_QTXML_LIBRARY}
  ${QT_QTTEST_LIBRARY}
)
