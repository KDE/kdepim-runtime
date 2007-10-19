
include_directories(
	${CMAKE_SOURCE_DIR}/
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${Boost_INCLUDE_DIR}
)


########### next target ###############

set( akonadisearchprovider_LIB_SRC
	searchproviderbase.cpp
)

qt4_add_dbus_adaptor( akonadisearchprovider_LIB_SRC org.kde.Akonadi.SearchProvider.xml searchproviderbase.h Akonadi::SearchProvider )
qt4_add_dbus_interfaces( akonadisearchprovider_LIB_SRC ../server/interfaces/org.kde.Akonadi.Tracer.xml )

qt4_wrap_cpp(akonadisearchprovider_LIB_SRC searchprovider.h)
kde4_add_library(
        akonadisearchprovider SHARED
        ${akonadisearchprovider_LIB_SRC}
)


target_link_libraries( akonadisearchprovider ${KDE4_KDECORE_LIBS})
set_target_properties( akonadisearchprovider PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadisearchprovider  DESTINATION ${LIB_INSTALL_DIR})


########### next target ###############

macro_optional_find_package(Nepomuk)

if ( Nepomuk_FOUND )

set( messagesearchprovider_SRCS
	messagesearchprovider.cpp
)


kde4_add_executable(akonadi_message_searchprovider RUN_UNINSTALLED ${messagesearchprovider_SRCS})

target_link_libraries(akonadi_message_searchprovider akonadi akonadisearchprovider ${QT_QTCORE_LIBRARIES} ${QT_QTDBUS_LIBRARIES} ${KDE4_KMIME_LIBS} ${NEPOMUK_LIBRARIES})

install(TARGETS akonadi_message_searchprovider DESTINATION ${BIN_INSTALL_DIR})
install(FILES messagesearchprovider.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/searchproviders")

endif ( Nepomuk_FOUND )

########### install files ###############

# install( FILES
# 	searchprovider.h
# 	searchproviderbase.h
# DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi
# )
