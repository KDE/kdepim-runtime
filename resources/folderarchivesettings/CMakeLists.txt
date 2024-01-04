add_definitions(-DTRANSLATION_DOMAIN=\"libfolderarchivesettings\")

add_library(folderarchivesettings)
target_sources(folderarchivesettings PRIVATE
   folderarchivesettingpage.cpp
   folderarchiveutil.cpp
   folderarchiveaccountinfo.cpp 
   folderarchivesettingpage.h
   folderarchiveutil.h
   folderarchiveaccountinfo.h
   folderarchivecombobox.h
   folderarchivecombobox.cpp
)

generate_export_header(folderarchivesettings BASE_NAME folderarchivesettings)
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(folderarchivesettings PROPERTIES UNITY_BUILD ON)
endif()

target_link_libraries(folderarchivesettings
PRIVATE
                      KPim6::Mime
                      KPim6::AkonadiCore
                      KPim6::AkonadiWidgets
                      KF6::ConfigCore
                      KF6::I18n
                      Qt::DBus
)

set_target_properties(folderarchivesettings PROPERTIES VERSION ${KDEPIMRUNTIME_LIB_VERSION} SOVERSION ${KDEPIMRUNTIME_LIB_SOVERSION} )

install(TARGETS folderarchivesettings ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
if (BUILD_TESTING)
   add_subdirectory(autotests)
endif()

