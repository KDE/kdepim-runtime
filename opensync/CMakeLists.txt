include ( OpenSyncInternal )
include ( OpenSyncPlugin )

LINK_DIRECTORIES(  ${OPENSYNC_LIBRARY_DIRS} ${GLIB2_LIBRARY_DIRS} )
INCLUDE_DIRECTORIES( ${OPENSYNC_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS}  ${Boost_INCLUDE_DIR} )

set( akonadi_opensync_srcs
  akonadi_opensync.cpp
  sinkbase.cpp
  akonadisink.cpp
  datasink.cpp
  calendarsink.cpp
  contactsink.cpp
)

# for boost
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

automoc4( akonadi_opensync_plugin akonadi_opensync_srcs )
OPENSYNC_PLUGIN_ADD( akonadi_opensync_plugin ${akonadi_opensync_srcs} )

TARGET_LINK_LIBRARIES( akonadi_opensync_plugin
  ${OPENSYNC_LIBRARIES}
  ${GLIB2_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_AKONADI_LIBS}
  ${KDE4_KCAL_LIBS}
)

OPENSYNC_PLUGIN_INSTALL( akonadi_opensync_plugin )
OPENSYNC_PLUGIN_CONFIG( akonadi-sync )
