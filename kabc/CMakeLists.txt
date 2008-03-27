add_subdirectory(kcontactmanager)

include_directories(
	${CMAKE_SOURCE_DIR}/
	${CMAKE_SOURCE_DIR}/akonadi
	${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( kabcakonadi_LIB_SRC
  itembrowser.cpp
  kabcitembrowser.cpp
  kabcitemeditor.cpp
  kabcmodel.cpp
)

qt4_wrap_ui(kabcakonadi_LIB_SRC kabcitemeditor.ui)
kde4_add_library( akonadi-kabc SHARED ${kabcakonadi_LIB_SRC} )

target_link_libraries( akonadi-kabc ${AKONADI_LIBS} ${KDE4_KABC_LIBS} )
set_target_properties( akonadi-kabc PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadi-kabc DESTINATION ${LIB_INSTALL_DIR})

########### install files ###############

install( FILES
  akonadi-kabc_export.h
  itembrowser.h
  kabcitembrowser.h
  kabcitemeditor.h
  kabcmodel.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/kabc
)
