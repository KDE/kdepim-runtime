include_directories(${CMAKE_SOURCE_DIR}/akonadi)
include_directories(${CMAKE_SOURCE_DIR}/akonadi/libakonadi)
include_directories(${CMAKE_SOURCE_DIR}/libkdepim)

set(akonamail_bin_SRCS
    main.cpp
    mainwidget.cpp
    mainwindow.cpp
)

qt4_add_dbus_interfaces( akonamail_bin_SRCS
  ../../server/interfaces/org.kde.Akonadi.TracerNotification.xml
)

kde4_add_executable(akonamail_bin ${akonamail_bin_SRCS})
set_target_properties(akonamail_bin PROPERTIES OUTPUT_NAME akonamail)

target_link_libraries( akonamail_bin ${AKONADI_LIBS} ${AKONADI_KMIME_LIBS} kdepim ${KDE4_KDEUI_LIBRARY} )

install(TARGETS akonamail_bin DESTINATION ${BIN_INSTALL_DIR} )
