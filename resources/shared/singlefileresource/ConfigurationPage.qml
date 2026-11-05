// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.akonadi as Akonadi

Akonadi.AgentConfigurationPage {
    id: root

    readonly property SingleFileResourceConfigQuickBase settings: configuration.controller as SingleFileResourceConfigQuickBase

    KI18nContext {
        id: _tr
        translationDomain: "akonadi_singlefile_resource"
    }

    title: _tr.i18nc("@title:window", "File Resource Configuration")

    header: Kirigami.InlineMessage {
        position: Kirigami.InlineMessage.Position.Header
        visible: text.length > 0
        text: root.settings.status
        type: root.settings.statusIsError ? Kirigami.MessageType.Error : Kirigami.MessageType.Information
    }

    FormCard.FormCard {
        Layout.topMargin: Kirigami.Units.gridUnit
        autoSeparators: true

        FormCard.FormTextFieldDelegate {
            label: _tr.i18nc("@label:textbox", "File or URL:")
            text: root.settings.path
            enabled: root.settings.pathEditable
            onTextEdited: root.settings.path = text
            trailing: Controls.ToolButton {
                icon.name: "document-open"
                text: _tr.i18nc("@action:button", "Select File")
                display: Controls.AbstractButton.IconOnly
                enabled: root.settings.pathEditable
                onClicked: fileDialogLoader.active = true
                Controls.ToolTip.text: text
                Controls.ToolTip.visible: hovered
            }
	    description: root.settings.pathEditable
                ? _tr.i18n("Select a local file or a remote URL to download a file from. A new local file will be created if it doesn't exist.")
                : _tr.i18n("To change the file location, delete this resource and create a new one with the updated path.")
        }

        FormCard.FormTextFieldDelegate {
            label: _tr.i18nc("@label:textbox", "Display Name:")
            text: root.settings.displayName
            onTextEdited: root.settings.displayName = text
        }

        FormCard.FormCheckDelegate {
            text: _tr.i18nc("@option:check", "Read only")
            description: _tr.i18n("When read-only mode is enabled, no changes will be written. Remote files are always read-only.")
            enabled: root.settings.localFile
            checked: root.settings.readOnly
            onToggled: root.settings.readOnly = checked
        }

        FormCard.FormCheckDelegate {
            text: _tr.i18nc("@option:check", "Monitor for changes")
            description: _tr.i18n("Reload the file when another application changes it. Monitoring is not available for remote URLs.")
            enabled: root.settings.localFile
            checked: root.settings.monitorFile
            onToggled: root.settings.monitorFile = checked
        }

        FormCard.FormCheckDelegate {
            text: _tr.i18nc("@option:check", "Update automatically")
            description: _tr.i18n("Periodically updates the file. Can be used when monitoring is not available (e.g. for remote calendars).")
            checked: root.settings.periodicUpdate
            onToggled: root.settings.periodicUpdate = checked
        }

        FormCard.FormSpinBoxDelegate {
            label: _tr.i18nc("@label:spinbox", "Update interval (minutes):")
            from: 1
            to: 1440
            enabled: root.settings.periodicUpdate
            value: root.settings.updatePeriod
            onValueModified: root.settings.updatePeriod = value
        }
    }


    Loader {
        id: fileDialogLoader
        active: false
        sourceComponent: FileDialog {
            fileMode: FileDialog.SaveFile
            options: FileDialog.DontConfirmOverwrite
            nameFilters: root.settings.nameFilters
            onAccepted: {
                root.settings.setFileUrl(selectedFile);
                fileDialogLoader.active = false;
            }
            onRejected: fileDialogLoader.active = false
            Component.onCompleted: open()
        }
    }
}
