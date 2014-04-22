project(folderarchivesettings)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${KDEPIMLIBS_INCLUDE_DIR}
)

include(KDE4Defaults)

kde_enable_exceptions()

set(folderarchivesettings_SRCS
   folderarchivesettingpage.cpp
   folderarchiveutil.cpp
   folderarchiveaccountinfo.cpp 
)

kde4_add_library(folderarchivesettings ${LIBRARY_TYPE} ${folderarchivesettings_SRCS} )

target_link_libraries(folderarchivesettings
                      KF5::KMime
                      ${AKONADI_COMMON_LIBRARIES}
                      ${QT_QTGUI_LIBRARY}
                      KF5::AkonadiCore
)

set_target_properties(folderarchivesettings PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )

install(TARGETS folderarchivesettings ${INSTALL_TARGETS_DEFAULT_ARGS})

