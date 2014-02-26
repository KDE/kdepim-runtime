include_directories(${LibKGAPI2_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

if(${AccountsQt_FOUND} AND ${SignOnQt_FOUND})
  include_directories(${ACCOUNTSQT_INCLUDE_DIRS} ${SIGNONQT_INCLUDE_DIRS} ../)
  add_definitions(-DHAVE_ACCOUNTS)
  set(accounts_SRCS ../../shared/getcredentialsjob.cpp)
endif()

macro_optional_add_subdirectory(calendar)
macro_optional_add_subdirectory(contacts)
