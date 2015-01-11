project(facebook-notifications-serializer)

set( SHARED_MIME_INFO_MINIMUM_VERSION "0.40" )
find_package( SharedMimeInfo REQUIRED )

set(akonadi_serializer_socialnotification_SRCS
  akonadi_serializer_socialnotification.cpp
)

add_library(akonadi_serializer_socialnotification MODULE ${akonadi_serializer_socialnotification_SRCS})

target_link_libraries(akonadi_serializer_socialnotification
  Qt5::Core
  KF5::AkonadiCore
  KF5::FbAPI
)

install(
  TARGETS akonadi_serializer_socialnotification
  DESTINATION ${KDE_INSTALL_PLUGINDIR}
)

install(
  FILES akonadi_serializer_socialnotification.desktop
  DESTINATION ${KDE_INSTALL_DATADIR}/akonadi/plugins/serializer
)

install(
  FILES x-vnd.akonadi.socialnotification.xml
  DESTINATION ${KDE_INSTALL_MIMEDIR}
)

update_xdg_mimetypes(${KDE_INSTALL_MIMEDIR})
