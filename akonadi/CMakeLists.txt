project(akonadi_playground)

#add_definitions( -DQT_NO_CAST_FROM_ASCII )
#add_definitions( -DQT_NO_CAST_TO_ASCII )

find_package(KDE4 REQUIRED)
find_package(Akonadi REQUIRED)
find_package(KdepimLibs REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/ )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

include_directories(
    akonadi_next
    ${AKONADI_INCLUDE_DIR}
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
add_subdirectory(pop3)

# This isn't released yet, sorry...
find_package(Grantlee)

if (Grantlee_FOUND)
  message("Found Grantlee. Building KJots.")

  #### KJots commmon stuff ####
  add_subdirectory(kjots_common)

  add_subdirectory(kjots_serializer)

  add_subdirectory(kjots_resource)

  add_subdirectory(kjotsapp)
endif (Grantlee_FOUND)


#### Clients ####

#add_subdirectory(plasmoid)
add_subdirectory(example_mail)
add_subdirectory(example_contacts)
#add_subdirectory(example_rss)
