include_directories(
	${CMAKE_SOURCE_DIR}/
	${CMAKE_SOURCE_DIR}/akonadi
	${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( akonadimodels_LIB_SRC
  kabcmodel.cpp
)

kde4_automoc(${akonadimodels_LIB_SRC})
kde4_add_library( akonadimodels SHARED ${akonadimodels_LIB_SRC} )

target_link_libraries( akonadimodels akonadi ${KDE4_KABC_LIBS} )
set_target_properties( akonadimodels PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS akonadimodels  DESTINATION ${LIB_INSTALL_DIR})

########### install files ###############

install( FILES
  kabcmodel.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/libakonadi
)
