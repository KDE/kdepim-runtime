project(folderarchivesettings)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${KDEPIMLIBS_INCLUDE_DIR}
)

include(KDE4Defaults)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

set(folderarchivesettings_SRCS
   folderarchivesettingpage.cpp
   folderarchiveutil.cpp
   folderarchiveaccountinfo.cpp 
)

kde4_add_library(folderarchivesettings ${LIBRARY_TYPE} ${folderarchivesettings_SRCS} )

target_link_libraries(folderarchivesettings
                      ${KDEPIMLIBS_KMIME_LIBS}
                      ${AKONADI_COMMON_LIBRARIES}
                      ${QT_QTGUI_LIBRARY}
                      ${KDEPIMLIBS_AKONADI_LIBS}
)

set_target_properties(folderarchivesettings PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )

install(TARGETS folderarchivesettings ${INSTALL_TARGETS_DEFAULT_ARGS})
add_subdirectory(autotests)

