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

macro( akonadi_add_resourcetest _testname _script )
  if ( ${EXECUTABLE_OUTPUT_PATH} )
    set( _exepath ${EXECUTABLE_OUTPUT_PATH} )
  else ( ${EXECUTABLE_OUTPUT_PATH} )
    set( _exepath ${CMAKE_CURRENT_BINARY_DIR}/.. )
  endif ( ${EXECUTABLE_OUTPUT_PATH} )
  if (WIN32)
    set(_resourcetester ${_exepath}/resourcetester.bat)
  else (WIN32)
    set(_resourcetester ${_exepath}/resourcetester)
  endif (WIN32)
  if (UNIX)
    set(_resourcetester ${_resourcetester}.shell)
  endif (UNIX)

  add_test( ${_testname} ${_resourcetester} -c ${CMAKE_CURRENT_SOURCE_DIR}/${_script} )
endmacro( akonadi_add_resourcetest )

add_subdirectory( tests )

set( resourcetester_SRCS
  global.cpp
  main.cpp
  resource.cpp
  resourcesynchronizationjob.cpp
  script.cpp
  test.cpp
  xmloperations.cpp
  collectiontest.cpp
  itemtest.cpp
  qemu.cpp
  system.cpp
  wrappedobject.cpp
)

kde4_add_executable(resourcetester RUN_UNINSTALLED ${resourcetester_SRCS})

target_link_libraries( resourcetester
  akonadi-xml
  ${KDE4_AKONADI_LIBS}
  ${QT_QTXML_LIBRARY}
  ${QT_QTTEST_LIBRARY}
  ${KDE4_KROSSUI_LIBS}
)
