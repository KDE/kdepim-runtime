project(akonadi_filter_framework)

#add_definitions( -DQT_NO_CAST_FROM_ASCII )
#add_definitions( -DQT_NO_CAST_TO_ASCII )

find_package(KDE4 REQUIRED)
find_package(Akonadi REQUIRED)
find_package(KdepimLibs REQUIRED)

include_directories(
    ${AKONADI_INCLUDE_DIR}
    ${KDE4_INCLUDES}
    ${KDEPIMLIBS_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)
include(CheckIncludeFiles)
include(FindKdepimLibs)

add_subdirectory(akonadi)
add_subdirectory(agent)
add_subdirectory(console)

add_custom_target(
	docs
	@echo Generating api documentation
)

add_custom_command(
	COMMENT "generate the api documentation"
	COMMAND mkdir -p ${CMAKE_CURRENT_SOURCE_DIR}/doc/api
	COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && doxygen ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
	TARGET docs
)

