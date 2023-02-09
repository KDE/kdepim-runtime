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
    property bool valid: vcarddirPath.text.length > 0
    Kirigami.FormLayout {
        RowLayout {
            Kirigami.FormData.label: i18n("Path:")
            QQC2.TextField {
                id: vcarddirPath
                Layout.fillWidth: true
                text: ""
                // TODO add ${VCARDDIR_FILE_DEFAULT_PATH} how ?
            }
            QQC2.Button {
                QQC2.ToolTip.text: i18n("Choose a folder")
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.shortDuration
                icon.name: "document-open-folder"
                onClicked: fileDialog.open()
                FileDialog {
                    id: fileDialog
                    title: i18n("Please choose a file")
                    folder: shortcuts.home
                    selectFolder: true
                    onAccepted: vcarddirPath.text = fileDialog.fileUrl
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
                    const vcardRes = SetupManager.createResource( "akonadi_vcarddir_resource" );
                    vcardRes.name = i18n("Default Contact");
                    vcardRes.setOption( "Path", vcarddirPath.text );

                    SetupManager.execute();
                }
            }
        }
    }
}
