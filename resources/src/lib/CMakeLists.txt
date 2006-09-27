
include_directories(
	${CMAKE_SOURCE_DIR}/
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${CMAKE_SOURCE_DIR}/akonadi/resources/include
)


########### next target ###############

set( akonadiresources_LIB_SRC
  kcrash.cpp
	resourcebase.cpp
)

set( akonadiresources_automoc_LIB_SRC
	resourceadaptor.cpp
)

set( akonadiresources_manualmoc_LIB_SRC
        ../../include/resource.h
        ../../include/resourcebase.h
)
qt4_add_dbus_interfaces( akonadiresources_automoc_LIB_SRC ../../../server/interfaces/org.kde.Akonadi.Tracer.xml )

qt4_wrap_cpp(akonadiresources_LIB_SRC ${akonadiresources_manualmoc_LIB_SRC})
kde4_automoc( ${akonadiresources_automoc_LIB_SRC} )
kde4_add_library(
        akonadiresources SHARED 
        ${akonadiresources_LIB_SRC} 
        ${akonadiresources_automoc_LIB_SRC} 
)


target_link_libraries( akonadiresources ${KDE4_KDECORE_LIBS} akonadi)
set_target_properties( akonadiresources PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS akonadiresources  DESTINATION ${LIB_INSTALL_DIR})

########### install files ###############

install( FILES ../../include/resource.h
               ../../include/resourcebase.h
               ../../include/org.kde.Akonadi.Resource.xml
DESTINATION ${INCLUDE_INSTALL_DIR}/libakonadiresources 
)


