// SPDX-FileCopyrightText: 2009-2023 Montel Laurent <montel@kde.org>
// 
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.15 as Kirigami
import org.kde.pim.accountwizard 1.0

Kirigami.ScrollablePage {
    id: root
    readonly property bool valid: mailboxPath.text.length > 0
    Kirigami.FormLayout {
        RowLayout {
            Kirigami.FormData.label: i18n("URL:")
            QQC2.TextField {
                id: mailboxPath
                Layout.fillWidth: true
                text: ""
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
                    onAccepted: mailboxPath.text = fileDialog.fileUrl
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
                    const mboxRes = SetupManager.createResource( "akonadi_mbox_resource" );
                    mboxRes.setOption( "Path", mailboxPath.text );

                    SetupManager.execute();
                }
            }
        }
    }
}
