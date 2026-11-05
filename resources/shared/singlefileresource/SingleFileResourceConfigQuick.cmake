# SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
# SPDX-License-Identifier: BSD-3-Clause

set(_singlefileresource_quick_dir ${CMAKE_CURRENT_LIST_DIR})

# The base class is compiled into each plugin so its QML type is registered in that plugin's module.
function(add_singlefileresource_config_quick target)
    cmake_parse_arguments(ARG "" "URI" "SOURCES;QML_FILES;LINK_LIBRARIES" ${ARGN})

    kcoreaddons_add_plugin(${target} INSTALL_NAMESPACE "pim6/akonadi/config")

    target_sources(${target} PRIVATE
        ${ARG_SOURCES}
        ${_singlefileresource_quick_dir}/singlefileresourceconfigquickbase.cpp
        ${_singlefileresource_quick_dir}/singlefileresourceconfigquickbase.h
    )

    set_source_files_properties(${_singlefileresource_quick_dir}/ConfigurationPage.qml
        PROPERTIES QT_RESOURCE_ALIAS ConfigurationPage.qml
    )

    qt_add_qml_module(${target}
        URI ${ARG_URI}
        VERSION 1.0
        RESOURCE_PREFIX /qt/qml
        NO_PLUGIN
        DEPENDENCIES org.kde.akonadi org.kde.ki18n
        QML_FILES ${_singlefileresource_quick_dir}/ConfigurationPage.qml ${ARG_QML_FILES}
    )

    target_link_libraries(${target} PRIVATE
        KF6::ConfigGui
        KF6::CoreAddons
        KF6::I18n
        KF6::KIOCore
        KPim6::AkonadiCore
        Qt6::Qml
        Qt6::Quick
        ${ARG_LINK_LIBRARIES}
    )
endfunction()
