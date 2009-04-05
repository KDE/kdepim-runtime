set(akonablog_bin_SRCS
    main.cpp
    mainwidget.cpp
    mainwindow.cpp
    blogmodel.cpp
    microblogdelegate.cpp
    akonaditabbar.cpp
)

kde4_add_app_icon(akonablog_bin_SRCS "${KDE4_ICON_DIR}/oxygen/*/apps/email.png")

kde4_add_executable(akonablog_bin ${akonablog_bin_SRCS})
set_target_properties(akonablog_bin PROPERTIES OUTPUT_NAME akonablog)

target_link_libraries(akonablog_bin ${KDE4_AKONADI_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTWEBKIT_LIBRARY} ${KDE4_KPIMUTILS_LIBS} ${KDE4_MICROBLOG_LIBS})

