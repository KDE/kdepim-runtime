include_directories( ${kdepim-runtime_SOURCE_DIR} )

set(akonamail_bin_SRCS
    main.cpp
    mainwidget.cpp
    mainwindow.cpp
)

kde4_add_app_icon(akonamail_bin_SRCS "${KDE4_ICON_DIR}/oxygen/*/apps/email.png")

kde4_add_executable(akonamail_bin ${akonamail_bin_SRCS})
set_target_properties(akonamail_bin PROPERTIES OUTPUT_NAME akonamail)

target_link_libraries( akonamail_bin ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${KDE4_KDEUI_LIBS} )

#install(TARGETS akonamail_bin ${INSTALL_TARGETS_DEFAULT_ARGS} )
