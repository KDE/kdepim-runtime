project(
		akonadi-filter-ui
	)

#add_subdirectory(
#		test
#	)

include_directories(
		${CMAKE_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}/../../../
	)

configure_file(
		config-akonadi-filter-ui.h.cmake
		${CMAKE_CURRENT_BINARY_DIR}/config-akonadi-filter-ui.h
	)

set(
		AKONADI_FILTER_UI_LIB_SRCS
		actioneditor.cpp
		actionselector.cpp
		conditionselector.cpp
		coolcombobox.cpp
		editorfactory.cpp
		extensionlabel.cpp
		expandingscrollarea.cpp
		programeditor.cpp
		ruleeditor.cpp
		rulelisteditor.cpp
		valueeditor.cpp
	)

kde4_add_library(
		akonadi-filter-ui
		SHARED
		${AKONADI_FILTER_UI_LIB_SRCS}
	)

target_link_libraries(
		akonadi-filter-ui
		akonadi-filter
		${KDE4_AKONADI_LIBS}
		ksieve
		${KDE4_KDECORE_LIBS}
		${KDE4_KDEUI_LIBS}
		${QT_LIBRARIES}
		${QT_QTCORE_LIBRARY}
		${QT_QTGUI_LIBRARY}
	)

set_target_properties(
		akonadi-filter-ui
		PROPERTIES
			VERSION
				${GENERIC_LIB_VERSION}
			SOVERSION
				${GENERIC_LIB_SOVERSION}
	)

add_definitions(
		-DBUILD_AKONADI_FILTER_UI_EXPORT_LIBRARY
	)

install(
		TARGETS
			akonadi-filter-ui
		EXPORT
			kdepimlibsLibraryTargets
		${INSTALL_TARGETS_DEFAULT_ARGS}
	)

install(
		FILES
			actionselector.h
			conditionselector.h
			programeditor.h
			ruleeditor.h
			rulelisteditor.h
		DESTINATION
			${INCLUDE_INSTALL_DIR}/akonadi/filter/ui
		COMPONENT
			Devel
	)
