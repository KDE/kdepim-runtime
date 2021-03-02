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
    property bool valid: incommingAddressField.text.trim().length !== 0
    property int stage: 1;
    property var identity;
    
    Component.onCompleted: {
        // try to guess some defaults
        const emailAddr = SetupManager.email();
        const pos = emailAddr.indexOf("@");
        if (pos >= 0 && (pos + 1) < emailAddr.length) {
            const server = emailAddr.slice( pos + 1, emailAddr.length );
            incommingAddressField.text = server;
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

            ServerTest.test(incommingAddressField.text.trim(), "imap");
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
            var imapRes = SetupManager.createResource("akonadi_imap_resource");
            var server = incommingAddressField.text.trim();
            imapRes.setOption("ImapServer", server);
            imapRes.setOption("UserName", nameField.text);
            imapRes.setOption("Password", SetupManager.password);
            imapRes.setOption("DisconnectedModeEnabled", disconnectedModeBox.checked);
            imapRes.setOption("UseDefaultIdentity", false);
            imapRes.setOption("AccountIdentity", identity.uoid());
            if ( server == "imap.gmail.com" ) {
                imapRes.setOption("Authentication", 9); // XOAuth2
                arg = "ssl";
            } else {
                imapRes.setOption("Authentication", 7); // ClearText
            }
            if ( arg == "ssl" ) { 
                // The ENUM used for authentication (in the imap resource only)
                imapRes.setOption("Safety", "SSL"); // SSL/TLS
                imapRes.setOption("ImapPort", 993);
            } else if ( arg == "tls" ) { // tls is really STARTTLS
                imapRes.setOption("Safety", "STARTTLS");  // STARTTLS
                imapRes.setOption("ImapPort", 143);
            } else if ( arg == "none" ) {
                imapRes.setOption("Safety", "NONE");  // No encryption
                imapRes.setOption("ImapPort", 143);
            } else {
                // safe default fallback when servertest failed
                imapRes.setOption("Safety", "STARTTLS");
                imapRes.setOption("ImapPort", 143 );
            }
            imapRes.setOption("IntervalCheckTime", 60);
            imapRes.setOption("SubscriptionEnabled", true);

            stage = 2;
            setup();
        } else {
            var smtp = SetupManager.createTransport("smtp");
            smtp.name = outgoingAddressField.text.trim();
            smtp.host = outgoingAddressField.text.trim();
            if (arg === "ssl") { 
                smtp.encryption = "SSL";
            } else if (arg === "tls") {
                smtp.encryption = "TLS";
            } else {
                smtp.encryption = "None";
            }
            smtp.username = nameField.text;
            smtp.password = SetupManager.password;
            identity.setTransport(smtp);
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
        QQC2.CheckBox {
            id: disconnectedModeBox
            text: i18n("&Download all messages for offline use")
        }
    }
    
    footer: QQC2.ToolBar {
        contentItem: RowLayout {
            QQC2.Button {
                text: i18n("Next")
                enabled: root.valid
                onClicked: setup()
            }
        }
    }
}
