
include_directories(
	${CMAKE_SOURCE_DIR}/
	${CMAKE_SOURCE_DIR}/libakonadi
	${CMAKE_SOURCE_DIR}/libkmime
	${CMAKE_SOURCE_DIR}/akonadiresources/include
)


########### next target ###############

set( akonadiresources_LIB_SRC
	lib/resourcebase.cpp
)

set( akonadiresources_automoc_LIB_SRC
)

set( akonadiresources_manualmoc_LIB_SRC
        ../include/resource.h
        ../include/resourcebase.h
)

qt4_wrap_cpp(akonadiresources_LIB_SRC ${akonadiresources_manualmoc_LIB_SRC})
kde4_automoc( ${akonadiresources_automoc_LIB_SRC} )
kde4_add_library(
        akonadiresources SHARED 
        ${akonadiresources_LIB_SRC} 
        ${akonadiresources_automoc_LIB_SRC} 
)


target_link_libraries( akonadiresources ${KDE4_KDECORE_LIBS} )
set_target_properties( akonadiresources PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install_targets( ${LIB_INSTALL_DIR} akonadiresources )

########### install files ###############

install_files( /include/libakonadiresources FILES
        ../../include/resource.h
        ../../include/resourcebase.h
)


