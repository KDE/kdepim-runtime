project(
		akonadi-filter-test
	)

set(
		AKONADI_FILTER_TEST_SRCS
		main.cpp
	)

include_directories(
		${CMAKE_SOURCE_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}/../
	)

kde4_add_executable(
		akonadi-filter-test
		${AKONADI_FILTER_TEST_SRCS}
	)

target_link_libraries(
		akonadi-filter-test
		akonadi-filter
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





