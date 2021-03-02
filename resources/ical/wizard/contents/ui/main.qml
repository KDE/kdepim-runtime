/*
    SPDX-FileCopyrightText: 2010 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Dialogs 1.3
import org.kde.kirigami 2.15 as Kirigami
import org.kde.pim.accountwizard 1.0

Kirigami.ScrollablePage {
    id: root
    property bool valid: pathField.text.length > 0
    Kirigami.FormLayout {
        RowLayout {
            Kirigami.FormData.label: i18n("File path:")
            QQC2.TextField {
                id: pathField
                Layout.fillWidth: true
                text: "$HOME/.local/share/korganizer/calendar.ics"
            }
            QQC2.Button {
                QQC2.ToolTip.text: i18n("Choose a file")
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.shortDuration
                icon.name: "document-open-folder"
                onClicked: fileDialog.open()
                FileDialog {
                    id: fileDialog
                    title: i18n("Please choose a file")
                    folder: shortcuts.home
                    onAccepted: pathField = fileDialog.fileUrl
                }   
            }
        }
    }
    
    footer: QQC2.ToolBar {
        contentItem: RowLayout {
            QQC2.Button {
                Layout.alignment: Qt.AlignRight
                text: i18n("Next")
                enabled: root.valid
                onClicked: {
                    const icalRes = SetupManager.createResource("akonadi_ical_resource");
                    icalRes.setOption("Path", pathFiel.text);
                    icalRes.setName(i18n("Default Calendar"));
                    SetupManager.execute();
                }
            }
        }
    }
}
