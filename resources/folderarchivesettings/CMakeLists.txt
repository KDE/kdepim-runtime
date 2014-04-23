project(folderarchivesettings)
kde_enable_exceptions()

set(folderarchivesettings_SRCS
   folderarchivesettingpage.cpp
   folderarchiveutil.cpp
   folderarchiveaccountinfo.cpp 
)

add_library(folderarchivesettings ${LIBRARY_TYPE} ${folderarchivesettings_SRCS} )

target_link_libraries(folderarchivesettings
                      KF5::KMime
                      KF5::AkonadiCore
                      KF5::AkonadiWidgets
)

set_target_properties(folderarchivesettings PROPERTIES VERSION ${KF5_VERSION} SOVERSION ${KF5_VERSION} )

install(TARGETS folderarchivesettings ${INSTALL_TARGETS_DEFAULT_ARGS})

