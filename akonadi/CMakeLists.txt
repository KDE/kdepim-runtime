project(akonadi_playground)

#add_definitions( -DQT_NO_CAST_FROM_ASCII )
#add_definitions( -DQT_NO_CAST_TO_ASCII )

find_package(KDE4 REQUIRED)
find_package(Akonadi REQUIRED)
find_package(KdepimLibs REQUIRED)

include_directories(
    akonadi_next
    ${AKONADI_INCLUDE_DIR}
    kjots_common
    ${KDE4_INCLUDES}
    ${KDEPIMLIBS_INCLUDE_DIR}
    ${QT_INCLUDES}
)


include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)
include(CheckIncludeFiles)
include(FindKdepimLibs)

add_subdirectory(akonadi_next)
add_subdirectory(xml)

#### KJots commmon stuff ####
add_subdirectory(kjots_common)

add_subdirectory(kjots_serializer)

add_subdirectory(kjots_resource)

#### Clients ####

#add_subdirectory(plasmoid)
add_subdirectory(kjotsapp)
add_subdirectory(example_mail)
add_subdirectory(example_contacts)
#add_subdirectory(example_rss)

