include_directories(${CMAKE_SOURCE_DIR}/akonadi)
include_directories(${CMAKE_SOURCE_DIR}/libkdepim)

set(akonamail_bin_SRCS
    main.cpp
    mainwidget.cpp
    mainwindow.cpp
)

kde4_add_app_icon(akonamail_bin_SRCS "${KDE4_ICON_DIR}/oxygen/*/apps/email.png")

kde4_add_executable(akonamail_bin ${akonamail_bin_SRCS})
set_target_properties(akonamail_bin PROPERTIES OUTPUT_NAME akonamail)

target_link_libraries( akonamail_bin ${KDE4_AKONADI_LIBS} ${KDE4_AKONADI_KMIME_LIBS} kdepim ${KDE4_KDEUI_LIBRARY} )

#install(TARGETS akonamail_bin ${INSTALL_TARGETS_DEFAULT_ARGS} )
