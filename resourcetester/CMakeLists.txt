

remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

#add_subdirectory( tests )

set( resourcetester_SRCS
  global.cpp
  main.cpp
  resource.cpp
  script.cpp
  test.cpp
  xmloperations.cpp
  collectiontest.cpp
  itemtest.cpp
  qemu.cpp
  system.cpp
  wrappedobject.cpp
)

add_executable(resourcetester ${resourcetester_SRCS})

target_link_libraries( resourcetester
  KF5::AkonadiXml
  KF5::AkonadiCore
  Qt5::Test
  KF5::KrossCore
  KF5::KIOCore
)

