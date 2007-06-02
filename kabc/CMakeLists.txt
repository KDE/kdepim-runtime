include_directories(
	${CMAKE_SOURCE_DIR}/
	${CMAKE_SOURCE_DIR}/akonadi
	${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( kabcakonadi_LIB_SRC
  kabcitembrowser.cpp
  kabcitemeditor.cpp
  kabcmodel.cpp
)

kde4_automoc(${kabcakonadi_LIB_SRC})
kde4_add_library( kabcakonadi SHARED ${kabcakonadi_LIB_SRC} )

target_link_libraries( kabcakonadi akonadi ${KDE4_KABC_LIBS} )
set_target_properties( kabcakonadi PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS kabcakonadi DESTINATION ${LIB_INSTALL_DIR})

########### install files ###############

install( FILES
  kabcitembrowser.h
  kabcitemeditor.h
  kabcmodel.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/libakonadi/kabc
)
