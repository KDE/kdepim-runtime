project(
		akonadi-filter
	)

add_subdirectory(
		test
	)

add_subdirectory(
		ui
	)

include_directories(
		${CMAKE_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}/../../
	)

configure_file(
		config-akonadi-filter.h.cmake
		${CMAKE_CURRENT_BINARY_DIR}/config-akonadi-filter.h
	)

set(
		AKONADI_FILTER_LIB_SRCS
		action.cpp
		agent.cpp
		commanddescriptor.cpp
		component.cpp
		condition.cpp
		data.cpp
		datamemberdescriptor.cpp
		componentfactory.cpp
		functiondescriptor.cpp
		operatordescriptor.cpp
		program.cpp
		rule.cpp
		rulelist.cpp
		io/afldecoder.cpp
		io/decoder.cpp
		io/encoder.cpp
		io/sievedecoder.cpp
		io/sieveencoder.cpp
		io/sievereader.cpp
	)

kde4_add_library(
		akonadi-filter
		SHARED
		${AKONADI_FILTER_LIB_SRCS}
	)

target_link_libraries(
		akonadi-filter
		${KDEPIMLIBS_AKONADI_LIBS}
		ksieve
		${KDE4_KDECORE_LIBS}
		${QT_LIBRARIES}
	)

set_target_properties(
		akonadi-filter
		PROPERTIES
			VERSION
				${GENERIC_LIB_VERSION}
			SOVERSION
				${GENERIC_LIB_SOVERSION}
	)

add_definitions(
		-DBUILD_AKONADI_FILTER_EXPORT_LIBRARY
	)

install(
		TARGETS
			akonadi-filter
		EXPORT
			kdepimlibsLibraryTargets
		${INSTALL_TARGETS_DEFAULT_ARGS}
	)

install(
		FILES
			${CMAKE_CURRENT_BINARY_DIR}/config-akonadi-filter.h
			action.h
			agent.h
			commanddescriptor.h
			component.h
			condition.h
			data.h
			datamemberdescriptor.h
			datatype.h
			componentfactory.h
			functiondescriptor.h
			operatordescriptor.h
			program.h
			rule.h
			rulelist.h
		DESTINATION
			${INCLUDE_INSTALL_DIR}/akonadi/filter
		COMPONENT
			Devel
	)

install(
		FILES
			io/afldecoder.h
			io/decoder.h
			io/encoder.h
			io/sievedecoder.h
			io/sieveencoder.h
		DESTINATION
			${INCLUDE_INSTALL_DIR}/akonadi/filter/io
		COMPONENT
			Devel
	)
