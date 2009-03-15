project (resourcetester)

find_package(KDE4 REQUIRED)
find_package(KdepimLibs REQUIRED)
find_package(Boost REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)
include(CheckIncludeFiles)

include_directories(
    ${AKONADI_INCLUDE_DIR}
    ${KDE4_INCLUDES}
    ${KDEPIMLIBS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)


set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set( resourcetester_SRCS
  test.cpp
  main.cpp
  resource.cpp
  script.cpp
  xmlcomparator.cpp
)

kde4_add_executable(resourcetester RUN_UNINSTALLED ${resourcetester_SRCS})

target_link_libraries( resourcetester
  akonadi-xml
  ${KDE4_AKONADI_LIBS}
  ${QT_QTXML_LIBRARY}
  ${QT_QTTEST_LIBRARY}
  ${KDE4_KROSSUI_LIBS}
  akonadi-xml
)
