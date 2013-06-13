include_directories(${LibKGAPI2_INCLUDE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/../        #getcredentialsjob.h
)

macro_optional_add_subdirectory(calendar)
macro_optional_add_subdirectory(contacts)

