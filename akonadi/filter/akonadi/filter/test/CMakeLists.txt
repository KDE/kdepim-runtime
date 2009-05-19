project(
		akonadi-filter-test
	)

set(
		AKONADI_FILTER_TEST_SRCS
		main.cpp
	)

# This path emulates the KDE include path when the library headers
# are actually installed.

include_directories(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../
	)

# we need these because we're including in-tree stuff which isn't installed yet

include_directories(
		${CMAKE_CURRENT_BINARY_DIR}/../../../
		${CMAKE_CURRENT_BINARY_DIR}/../../../akonadi/filter/
		${CMAKE_CURRENT_BINARY_DIR}/../../../akonadi/filter/ui/
	)


kde4_add_executable(
		akonadi-filter-test
		${AKONADI_FILTER_TEST_SRCS}
	)

target_link_libraries(
		akonadi-filter-test
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
			akonadi-filter-test
		DESTINATION
			${BIN_INSTALL_DIR}
	)





