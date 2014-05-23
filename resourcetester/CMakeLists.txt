
kde_enable_exceptions()

add_subdirectory( tests )

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
  ${KDEPIMLIBS_AKONADI_XML_LIBS}
  KF5::AkonadiCore
  
  
  ${QT_QTXML_LIBRARY}
  
  
  Qt5::Test
  KF5::KrossCore
  ${KDE4_KROSSUI_LIBS}
  
  
  KF5::KIOCore
)

