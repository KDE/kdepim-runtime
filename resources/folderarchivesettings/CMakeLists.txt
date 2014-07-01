project(folderarchivesettings)


add_definitions(-DTRANSLATION_DOMAIN=\"libfolderarchivesettings\")


set(folderarchivesettings_SRCS
   folderarchivesettingpage.cpp
   folderarchiveutil.cpp
   folderarchiveaccountinfo.cpp 
)

add_library(folderarchivesettings ${LIBRARY_TYPE} ${folderarchivesettings_SRCS} )

target_link_libraries(folderarchivesettings
                      KF5::Mime
                      KF5::AkonadiCore
                      KF5::AkonadiWidgets
)

set_target_properties(folderarchivesettings PROPERTIES VERSION ${KDEPIMRUNTIME_LIB_VERSION} SOVERSION ${KDEPIMRUNTIME_LIB_VERSION} )

install(TARGETS folderarchivesettings ${INSTALL_TARGETS_DEFAULT_ARGS})

