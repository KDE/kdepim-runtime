
include_directories(
	${CMAKE_SOURCE_DIR}/
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
)


########### next target ###############

set( akonadisearchprovider_LIB_SRC
	searchproviderbase.cpp
)

qt4_add_dbus_adaptor( akonadisearchprovider_LIB_SRC org.kde.Akonadi.SearchProvider.xml searchproviderbase.h Akonadi::SearchProvider )
qt4_add_dbus_interfaces( akonadisearchprovider_LIB_SRC ../server/interfaces/org.kde.Akonadi.Tracer.xml )

qt4_wrap_cpp(akonadisearchprovider_LIB_SRC searchprovider.h)
kde4_automoc(${akonadisearchprovider_LIB_SRC})
kde4_add_library(
        akonadisearchprovider SHARED
        ${akonadisearchprovider_LIB_SRC}
)


target_link_libraries( akonadisearchprovider ${KDE4_KDECORE_LIBS})
set_target_properties( akonadisearchprovider PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS akonadisearchprovider  DESTINATION ${LIB_INSTALL_DIR})


########### next target ###############

macro_optional_find_package(KMetaData)

if ( KMetaData_FOUND )

set( messagesearchprovider_SRCS
	messagesearchprovider.cpp
)

kde4_automoc(${messagesearchprovider_SRCS})

kde4_add_executable(akonadi_message_searchprovider RUN_UNINSTALLED ${messagesearchprovider_SRCS})

target_link_libraries(akonadi_message_searchprovider akonadi akonadisearchprovider ${QT_QTCORE_LIBRARIES} ${QT_QTDBUS_LIBRARIES} ${KDE4_KMIME_LIBS} ${KMETADATA_LIBRARIES})

install(TARGETS akonadi_message_searchprovider DESTINATION ${BIN_INSTALL_DIR})
install(FILES messagesearchprovider.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/apps/akonadi/searchproviders")

endif ( KMetaData_FOUND )

########### next target ###############

macro_optional_find_package(Strigi)

if ( STRIGI_FOUND )

include_directories( ${STRIGI_INCLUDE_DIR} )

set( strigiprovider_SRCS
	strigiprovider.cpp
)

kde4_automoc(${strigiprovider_SRCS})

kde4_add_executable(akonadi_strigi_searchprovider RUN_UNINSTALLED ${strigiprovider_SRCS})

target_link_libraries(akonadi_strigi_searchprovider akonadi akonadisearchprovider ${QT_QTCORE_LIBRARIES} ${QT_QTDBUS_LIBRARIES} ${STRIGIQTDBUSCLIENT_LIBRARY})

install(TARGETS akonadi_strigi_searchprovider DESTINATION ${BIN_INSTALL_DIR})
install(FILES strigisearchprovider.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/apps/akonadi/searchproviders")

endif ( STRIGI_FOUND )

########### install files ###############

# install( FILES
# 	searchprovider.h
# 	searchproviderbase.h
# DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi
# )
