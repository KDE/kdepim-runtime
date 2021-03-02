/*
    SPDX-FileCopyrightText: 2017 Montel Laurent <montel@kde.org>
    SPDX-FileCopyrightText: 2021 Carl Schwan <carlschan@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.15 as Kirigami
import org.kde.pim.accountwizard 1.0

Kirigami.Page {
    id: root
    Kirigami.PlaceholderMessage {
        text: i18n("This wizard only create an empty Microsoft Exchange Web Services account.")
        anchors.centerIn: parent
        helpfulAction: Kirigami.Action {
            text: i18n("OK")
            onTriggered: next.clicked();
        }
    }
    footer: QQC2.ToolBar {
        contentItem: RowLayout {
            QQC2.Button {
                id: next
                Layout.alignment: Qt.AlignRight
                text: i18n("Next")
                enabled: root.valid
                onClicked: {
                    var maildirRes = SetupManager.createResource( "akonadi_ews_resource" );
                    //maildirRes.setOption( "Path", page.widget().maildirPath.text );

                    SetupManager.execute();
                }
            }
        }
    }
}
