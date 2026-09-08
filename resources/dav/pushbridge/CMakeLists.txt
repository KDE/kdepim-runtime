# SPDX-FileCopyrightText: 2026 Benjamin Port <benjamin.port@enioka.com>

find_package(KUnifiedPush REQUIRED)

set(davpushnotifybridge_SRCS
    main.cpp
    davpushnotifybridge.cpp
    davpushnotifybridge.h
    resourcehandler.cpp
    resourcehandler.h
)

qt_add_dbus_adaptor(dbus_srcs interfaces/org.kde.akonadi_davpushnotifybridge.Management.xml davpushnotifybridge.h DavPushNotifyBridge)

ecm_qt_declare_logging_category(davpushnotifybridge_SRCS HEADER davpushnotifybridge_debug.h IDENTIFIER DAVPUSHNOTIFYBRIDGE_LOG CATEGORY_NAME org.kde.pim.davpushnotifybridge
    DESCRIPTION "dav push notify bridge (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME
)

add_executable(
    akonadi_davpushnotifybridge
    ${davpushnotifybridge_SRCS}
    ${dbus_srcs}
)

configure_file(
    org.kde.akonadi_davpushnotifybridge.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.akonadi_davpushnotifybridge.service
)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/org.kde.akonadi_davpushnotifybridge.service
    DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
)

target_link_libraries(
    akonadi_davpushnotifybridge
    KF6::CoreAddons
    KF6::Crash
    KF6::DBusAddons
    KUnifiedPush
)

install(
    TARGETS
        akonadi_davpushnotifybridge
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)
