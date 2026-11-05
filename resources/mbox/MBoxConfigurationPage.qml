// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n
import org.kde.kirigamiaddons.formcard as FormCard

ConfigurationPage {
    id: root

    readonly property MBoxConfigQuick mboxSettings: configuration.controller as MBoxConfigQuick

    KI18nContext {
        id: _tr
        translationDomain: "akonadi_mbox_resource"
    }

    FormCard.FormHeader {
        title: _tr.i18nc("@title:group", "Compact frequency")
    }

    FormCard.FormCard {
        autoSeparators: true

        FormCard.FormCheckDelegate {
            text: _tr.i18nc("@option:check", "Compact automatically")
            checked: root.mboxSettings.compactFrequency === 1
            onToggled: root.mboxSettings.compactFrequency = checked ? 1 : 0
        }

        FormCard.FormSpinBoxDelegate {
            label: _tr.i18nc("@label:spinbox", "Deleted messages before compaction:")
            from: 1
            to: 1000000
            enabled: root.mboxSettings.compactFrequency === 1
            value: root.mboxSettings.messageCount
            onValueModified: root.mboxSettings.messageCount = value
        }
    }

    FormCard.FormHeader {
        title: _tr.i18nc("@title:group", "Lock method")
    }

    FormCard.FormCard {
        autoSeparators: true

        Controls.ButtonGroup { id: lockGroup }

        FormCard.FormRadioDelegate {
            text: _tr.i18nc("@option:radio", "Procmail lockfile")
            enabled: root.mboxSettings.procmailAvailable
            checked: root.mboxSettings.lockfileMethod === 0
            Controls.ButtonGroup.group: lockGroup
            onClicked: root.mboxSettings.lockfileMethod = 0
        }

        FormCard.FormRadioDelegate {
            text: _tr.i18nc("@option:radio", "Mutt dotlock")
            enabled: root.mboxSettings.muttAvailable
            checked: root.mboxSettings.lockfileMethod === 1
            Controls.ButtonGroup.group: lockGroup
            onClicked: root.mboxSettings.lockfileMethod = 1
        }

        FormCard.FormRadioDelegate {
            text: _tr.i18nc("@option:radio", "Mutt dotlock (privileged)")
            enabled: root.mboxSettings.muttAvailable
            checked: root.mboxSettings.lockfileMethod === 2
            Controls.ButtonGroup.group: lockGroup
            onClicked: root.mboxSettings.lockfileMethod = 2
        }

        FormCard.FormRadioDelegate {
            text: _tr.i18nc("@option:radio", "None")
            checked: root.mboxSettings.lockfileMethod === 3
            Controls.ButtonGroup.group: lockGroup
            onClicked: root.mboxSettings.lockfileMethod = 3
        }

        FormCard.FormTextFieldDelegate {
            label: _tr.i18nc("@label:textbox", "Lock file:")
            enabled: root.mboxSettings.lockfileMethod === 0
            text: root.mboxSettings.lockfile
            onTextEdited: root.mboxSettings.lockfile = text
        }
    }
}
