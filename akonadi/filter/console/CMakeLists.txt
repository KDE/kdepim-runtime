project(
		akonadi_filter_console
	)

set(
		AKONADI_FILTER_CONSOLE_SRCS
		filter.cpp
		filtercollectionmodel.cpp
		filtereditor.cpp
		main.cpp
		mainwindow.cpp
		mimetypeselectiondialog.cpp
	)

# This path emulates the KDE include path when the library headers
# are actually installed.

include_directories(
		${CMAKE_CURRENT_SOURCE_DIR}/../
	)

# we need these because we're including in-tree stuff which isn't installed yet

include_directories(
		${CMAKE_CURRENT_BINARY_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}/../akonadi/filter/
		${CMAKE_CURRENT_BINARY_DIR}/../akonadi/filter/ui/
	)

# A little trick to force qt4_add_dbus_interface to add the correct include
# to the generated file
set(
		SAVE_QT_DBUSXML2CPP_EXECUTABLE
		${QT_DBUSXML2CPP_EXECUTABLE}
	)

set(
		QT_DBUSXML2CPP_EXECUTABLE
		${QT_DBUSXML2CPP_EXECUTABLE} -i akonadi/collection.h -i akonadi/filter/agent.h
	)

qt4_add_dbus_interface(
		AKONADI_FILTER_CONSOLE_SRCS
		../agent/org.freedesktop.Akonadi.FilterAgent.xml
		filteragentinterface
	)

set(
		QT_DBUSXML2CPP_EXECUTABLE
		${SAVE_QT_DBUSXML2CPP_EXECUTABLE}
	)

kde4_add_executable(
		akonadi_filter_console
		${AKONADI_FILTER_CONSOLE_SRCS}
	)

target_link_libraries(
		akonadi_filter_console
		akonadi-filter
		akonadi-filter-ui
		${KDEPIMLIBS_AKONADI_LIBS}
		${KDE4_KDECORE_LIBS}
		${QT_LIBRARIES}
		#${KDE4_KIO_LIBS}
		#${KDEPIMLIBS_KABC_LIBS}
		#${KDEPIMLIBS_KMIME_LIBS}
)

install(
		TARGETS
			akonadi_filter_console
		DESTINATION
			${BIN_INSTALL_DIR}
	)





