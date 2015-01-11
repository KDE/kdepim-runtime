project(folderarchivesettings)


add_definitions(-DTRANSLATION_DOMAIN=\"libfolderarchivesettings\")


set(folderarchivesettings_SRCS
   folderarchivesettingpage.cpp
   folderarchiveutil.cpp
   folderarchiveaccountinfo.cpp 
)

add_library(folderarchivesettings ${LIBRARY_TYPE} ${folderarchivesettings_SRCS} )
generate_export_header(folderarchivesettings BASE_NAME folderarchivesettings)

target_link_libraries(folderarchivesettings
                      KF5::Mime
                      KF5::AkonadiCore
                      KF5::AkonadiWidgets
)

set_target_properties(folderarchivesettings PROPERTIES VERSION ${KDEPIMRUNTIME_LIB_VERSION} SOVERSION ${KDEPIMRUNTIME_LIB_SOVERSION} )

install(TARGETS folderarchivesettings ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
add_subdirectory(autotests)

