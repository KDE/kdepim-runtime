include_directories(${LibKGAPI_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

macro_optional_add_subdirectory(calendar)
macro_optional_add_subdirectory(contacts)
macro_optional_add_subdirectory(reader)
