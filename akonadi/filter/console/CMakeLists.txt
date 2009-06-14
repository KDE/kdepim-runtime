project(
		akonadi_filter_console
	)

set(
		AKONADI_FILTER_CONSOLE_SRCS
		filter.cpp
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


qt4_add_dbus_interface(
		AKONADI_FILTER_CONSOLE_SRCS
		../agent/org.freedesktop.Akonadi.FilterAgent.xml
		filteragentinterface
	)

kde4_add_executable(
		akonadi_filter_console
		${AKONADI_FILTER_CONSOLE_SRCS}
	)

target_link_libraries(
		akonadi_filter_console
		akonadi-filter
		akonadi-filter-ui
		${KDE4_AKONADI_LIBS}
		${KDE4_KDECORE_LIBS}
		${QT_LIBRARIES}
		#${KDE4_KIO_LIBS}
		#${KDE4_KABC_LIBS}
		#${KDE4_KMIME_LIBS}
)

install(
		TARGETS
			akonadi_filter_console
		DESTINATION
			${BIN_INSTALL_DIR}
	)





