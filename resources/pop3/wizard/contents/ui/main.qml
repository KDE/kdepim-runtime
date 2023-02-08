/*
    SPDX-FileCopyrightText: 2009 Montel Laurent <montel@kde.org>
    SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.15 as Kirigami
import org.kde.pim.accountwizard 1.0

Kirigami.ScrollablePage {
    id: root
    property bool valid: incomingAddressField.text.trim().length !== 0
    property int stage: 1;
    property var identity;
    
    Component.onCompleted: {
        // try to guess some defaults
        const emailAddr = SetupManager.email();
        const pos = emailAddr.indexOf("@");
        if (pos >= 0 && (pos + 1) < emailAddr.length) {
            const server = emailAddr.slice( pos + 1, emailAddr.length );
            incomingAddressField.text = server;
            outgoingAddressField.text = server;
            // We must not strip the server from the user identifier.
            // Otherwise the 'user' will be kdabtest1 instead of kdabtest1@demo.kolab.org
            // which fails,
            //var user = emailAddr.slice( 0, pos );
            nameField.text = emailAddr;
        }

        try {
            ServerTest.testFail.connect(testResultFail);
            ServerTest.testResult.connect(testOk);
        } catch (e) {
            print(e);
        }
    }

    function setup() {
        if (stage === 1) {
            identity = SetupManager.createIdentity();
            identity.setEmail(SetupManager.email);
            identity.setRealName(SetupManager.name);

            ServerTest.test(incomingAddressField.text.trim(), "pop");
        } else {
            ServerTest.test(outgoingAddressField.text.trim(), "smtp");
        }
    }

    function testResultFail() {
        testOk(-1);
    }

    function testOk(arg) {
        if (stage == 1) {
            SetupManager.openWallet();
            var pop3Res = SetupManager.createResource("akonadi_pop3_resource");
            var server = incomingAddressField.text.trim();
            pop3Res.setOption( "Host", server );
            pop3Res.setOption( "Login", page.widget().userName.text.trim() );
            pop3Res.setOption( "Password", SetupManager.password() );
            if ( arg == "ssl" ) {
              pop3Res.setOption( "Port", 995 );
              pop3Res.setOption( "UseTLS", true );
            } else if ( arg == "tls" ) { // tls is really STARTTLS
              pop3Res.setOption( "Port", 110 );
              pop3Res.setOption( "UseTLS", true );
            } else if ( arg == "none" ) {
              pop3Res.setOption( "Port", 110 );
            } else {
              pop3Res.setOption( "Port", 110 );
            }

            stage = 2;
            setup();
        } else {
            var smtp = SetupManager.createTransport( "smtp" );
            smtp.setName( page.widget().outgoingAddress.text.trim() );
            smtp.setHost( page.widget().outgoingAddress.text.trim() );
            if ( arg == "ssl" ) {
                smtp.setEncryption( "SSL" );
            } else if ( arg == "tls" ) {
                smtp.setEncryption( "TLS" );
            } else {
                smtp.setEncryption( "None" );
            }
            smtp.setUsername( page.widget().userName.text );
            smtp.setPassword( SetupManager.password() );
            SetupManager.execute();
        }
    }

    Kirigami.FormLayout {
        Item {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Personal Settings")
        }
        QQC2.TextField {
            id: nameField
            Kirigami.FormData.label: i18n("Name:")
            text: SetupManager.name
        }
        QQC2.TextField {
            id: emailField
            Kirigami.FormData.label: i18n("Email:")
            onTextChanged: guessServerName()
            text: SetupManager.email
        }
        Kirigami.PasswordField {
            id: passwordField
            Kirigami.FormData.label: i18n("Password:")
            text: SetupManager.password
        }
        QQC2.TextField {
            id: incomingAddressField
            Kirigami.FormData.label: i18n("Incoming Address:")
        }
        QQC2.TextField {
            id: outgoingAddressField
            Kirigami.FormData.label: i18n("Outgoing Address:")
        }
    }
    
    footer: QQC2.ToolBar {
        contentItem: RowLayout {
            QQC2.Button {
                Layout.alignment: Qt.AlignRight
                text: i18n("Next")
                enabled: root.valid
                onClicked: setup()
            }
        }
    }
}
