include_directories(
        ${CMAKE_SOURCE_DIR}/
        ${CMAKE_SOURCE_DIR}/akonadi
        ${QT_QTDBUS_INCLUDE_DIR}
        ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( kcalakonadi_LIB_SRC
     kcalmimetypevisitor.cpp
     kcalmodel.cpp
     kcalitembrowser.cpp
     itembrowser.cpp
)

kde4_add_library( akonadi-kcal SHARED ${kcalakonadi_LIB_SRC} )

target_link_libraries( akonadi-kcal  ${KDE4_AKONADI_LIBS} ${KDE4_KCAL_LIBS} ${KDE4_KDEUI_LIBS} )
set_target_properties( akonadi-kcal PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadi-kcal ${INSTALL_TARGETS_DEFAULT_ARGS})
