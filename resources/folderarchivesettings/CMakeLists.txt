add_definitions(-DTRANSLATION_DOMAIN=\"libfolderarchivesettings\")


set(folderarchivesettings_SRCS
   folderarchivesettingpage.cpp
   folderarchiveutil.cpp
   folderarchiveaccountinfo.cpp 
)

add_library(folderarchivesettings  ${folderarchivesettings_SRCS} )
generate_export_header(folderarchivesettings BASE_NAME folderarchivesettings)
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(folderarchivesettings PROPERTIES UNITY_BUILD ON)
endif()

target_link_libraries(folderarchivesettings
PRIVATE
                      KF5::Mime
                      KF5::AkonadiCore
                      KF5::AkonadiWidgets
                      KF5::ConfigCore
                      KF5::I18n
                      Qt5::DBus
)

set_target_properties(folderarchivesettings PROPERTIES VERSION ${KDEPIMRUNTIME_LIB_VERSION} SOVERSION ${KDEPIMRUNTIME_LIB_SOVERSION} )

install(TARGETS folderarchivesettings ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
if (BUILD_TESTING)
   add_subdirectory(autotests)
endif()

