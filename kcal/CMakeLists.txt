include_directories(
        ${CMAKE_SOURCE_DIR}/
        ${CMAKE_SOURCE_DIR}/akonadi
        ${QT_QTDBUS_INCLUDE_DIR}
        ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( kcalakonadi_LIB_SRC
     kcalmodel.cpp
     kcalitembrowser.cpp
)


kde4_add_library( kcalakonadi SHARED ${kcalakonadi_LIB_SRC} )

target_link_libraries( kcalakonadi ${AKONADI_LIBS} ${KDE4_KCAL_LIBS} )
set_target_properties( kcalakonadi PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS kcalakonadi DESTINATION ${LIB_INSTALL_DIR})

########### install files ###############

install( FILES
         akonadi-kcal_export.h
         kcalmodel.h
	 kcalitembrowser.h
         DESTINATION ${INCLUDE_INSTALL_DIR}/libakonadi/kcal
)
