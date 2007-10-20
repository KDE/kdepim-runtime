
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


########### install files ###############

# install( FILES
# 	searchprovider.h
# 	searchproviderbase.h
# DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi
# )
