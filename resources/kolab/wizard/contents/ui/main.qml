// SPDX-FileCopyrightText: Sandro Knauß <knauss@kolabsys.com>
// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.15 as Kirigami
import org.kde.pim.accountwizard 1.0

Kirigami.ScrollablePage {
    id: root
    
    property bool userChangedServerAddress: false;
    readonly property bool valid: emailEdit.text.trim().length !== 0 && passwordEdit.text.trim().length !== 0
    
    Kirigami.FormLayout {
        QQC2.TextField {
            id: nameEdit
            Kirigami.FormData.label: i18n("Name:")
            text: SetupManager.name()
        }
        QQC2.TextField {
            id: emailEdit
            Kirigami.FormData.label: i18n("Email:")
            text: SetupManager.email()
        }
        Kirigami.PasswordField {
            id: passwordEdit
            Kirigami.FormData.label: i18n("Password:")
            text: SetupManager.password()
        }
        QQC2.ComboBox {
            Kirigami.FormData.label: i18n("Kolab Version")
            model: ["v2", "v3"]
        }
    }
    
    footer: QQC2.ToolBar {
        contentItem: RowLayout {
            QQC2.Button {
                Layout.alignment: Qt.AlignRight
                text: i18n("Next")
                enabled: root.valid
                onClicked: checkAutoconfig()
            }
        }
    }
    
    
    /** Server test */
    property bool servertest_running: false;
    /** 0 = not running, 1 = submission, 2 = smtp, 3 = imap */
    property int servertest_mode: 0;
    
    property var imapRes;
    function guessServerName() {
        const email = emailEdit.text;
        const pos = email.indexOf("@");
        if (pos >= 0 && (pos + 1) < email.length) {
            const server = email.slice(pos + 1, email.length);
            return server;
        }
        return "";
    }

    function testResultFail() {
        testOk( -1 );
    }

    function testOk(arg) {
        print("testOk arg =", arg);

        if (servertest_mode < 3) {   // submission & smtp
            if (arg == "tls" ) { // tls is really STARTTLS
            smtp.setEncryption("TLS");
            if (servertest_mode == 1) {   //submission port 587
                smtp.setPort(587);
            } else {
                smtp.setPort(25);
            }
            } else if ( arg == "ssl" ) {    //only possible as smtps
                smtp.setPort(465);
                smtp.setEncryption("SSL");
            } else if (servertest_mode == 2) { //test submission and smtp failed or only possible unencrypted -> set to standard value and open editor
                smtp.setPort(587);
                smtp.setEncryption("TLS");
                smtp.setEditMode(true);
            } else if (servertest_mode == 1) { // submission test failed -> start smtp request
                servertest_mode = 2;
                ServerTest.test(page2.widget().lineEditSmtp.text, "smtp");
                return;
            }

            // start imap test
            servertest_mode = 3;
            if (page2.widget().lineEditImap.text) {
                SetupManager.setupInfo(qsTr("Probing IMAP server..."));
                ServerTest.test(page2.widget().lineEditImap.text, "imap");
            } else {
                SetupManager.execute();
            }
        } else if (servertest_mode == 3) {   //imap
            if ( arg == "ssl" ) {
                // The ENUM used for authentication (in the kolab resource only)
                kolabRes.setOption("Safety", "SSL"); // SSL/TLS
                kolabRes.setOption("ImapPort", 993);
            } else if ( arg == "tls" ) { // tls is really STARTTLS
                kolabRes.setOption("Safety", "STARTTLS");  // STARTTLS
                kolabRes.setOption("ImapPort", 143);
            } else {
                // safe default fallback in case server test failed
                kolabRes.setOption("Safety", "STARTTLS");
                kolabRes.setOption("ImapPort", 143);
                kolabRes.setEditMode(true);
            }
            SetupManager.execute();
        } else {
            print ("Unknown servertest_mode = ", servertest_mode);
        }
    }

    property var ac_mail_stat;
    property var ac_freebusy_stat;
    property var ac_ldap_stat;
    
    Component {
        id: autoConfigurationPageComponent
        AutoConfigurationPage { }
    }
            
    
    function checkAutoconfig() {
        ac_mail_stat = false;
        ac_freebusy_stat = false;
        ac_ldap_stat = false;

        const autoConfigurationPage = QQC2.ApplicationWindow.window.pageStack.push(autoConfigurationPageComponent, {
            imapServerName: guessServerName(),
            smtpServerName: guessServerName(),
            imapEdit: false,
            smtpEdit: false,
        });
        
        /*
        
        page2.widget().lineEditImap.text = guessServerName();
        page2.widget().lineEditSmtp.text = guessServerName();
        page2.widget().lineEditImap.visible = false;
        page2.widget().lineEditSmtp.visible = false;
        page2.widget().lineEditImapLabel.visible = false;
        page2.widget().lineEditSmtpLabel.visible = false;

        page2.widget().checkBoxFreebusyEdit.text = qsTr("Create");
        page2.widget().checkBoxLdapEdit.text = qsTr("Create");

        ac_mail = SetupManager.ispDB('autoconfigkolabmail');
        ac_mail.ispdbFinished.connect(mail_finished);
        ac_mail.info.connect(mail_text);
        ac_mail.setEmail(page.widget().emailEdit.text);
        ac_mail.setPassword(page.widget().passwordEdit.password);
        ac_mail.start();

        ac_freebusy = SetupManager.ispDB('autoconfigkolabfreebusy');
        ac_freebusy.ispdbFinished.connect(freebusy_finished);
        ac_freebusy.info.connect(freebusy_text);
        ac_freebusy.setEmail(page.widget().emailEdit.text);
        ac_freebusy.setPassword(page.widget().passwordEdit.password);
        ac_freebusy.start();

        ac_ldap = SetupManager.ispDB('autoconfigkolabldap');
        ac_ldap.ispdbFinished.connect(ldap_finished);
        ac_ldap.info.connect(ldap_text);
        ac_ldap.setEmail(page.widget().emailEdit.text);
        ac_ldap.setPassword(page.widget().passwordEdit.password);
        ac_ldap.start();*/
    }
}/*

    var identity; // global so it can be accesed in setup and testOk

    var kolabRes;
    var smtp;
    var imapRes;

    var ac_mail;
    var ac_freebusy;
    var ac_ldap;


function mail_finished(stat) {
    ac_mail_stat = stat;
    if (stat) {
        page2.widget().lineEditImap.visible = false;
        page2.widget().lineEditSmtp.visible = false;
        page2.widget().lineEditImapLabel.visible = false;
        page2.widget().lineEditSmtpLabel.visible = false;
    } else {
        page2.widget().lineEditImap.visible = true;
        page2.widget().lineEditSmtp.visible = true;
        page2.widget().lineEditImapLabel.visible = true;
        page2.widget().lineEditSmtpLabel.visible = true;
    }
}

function mail_text(text) {
    page2.widget().labelImapSearch.text = text;
    page2.widget().labelSmtpSearch.text = text;
}

function freebusy_finished(stat) {
    ac_freebusy_stat = stat;
    if (stat) {
        page2.widget().checkBoxFreebusyEdit.text = qsTr("Manual Edit");
    }
}

function freebusy_text(text) {
    page2.widget().labelFreebusySearch.text = text;
}

function ldap_finished(stat) {
    ac_ldap_stat = stat;
    if (stat) {
        page2.widget().checkBoxLdapEdit.text = qsTr("Manual Edit");
    }
}

function ldap_text(text) {
    page2.widget().labelLdapSearch.text = text;
}

function setup()
{
    SetupManager.openWallet();
    smtp = SetupManager.createTransport("smtp");
    smtp.setEditMode(page2.widget().checkBoxSmtpEdit.checked);
    smtp.setPassword(page.widget().passwordEdit.password);

    if (ac_mail_stat) {
        ac_mail.fillSmtpServer(0, smtp);
    } else if (page2.widget().lineEditSmtp.text) {
        var serverAddress = page2.widget().lineEditSmtp.text;
        servertest_running = true;
        servertest_mode = 1;
        smtp.setName(serverAddress);
        smtp.setHost(serverAddress);
        smtp.setUsername(page.widget().emailEdit.text);
        smtp.setAuthenticationType("plain");

        SetupManager.setupInfo(qsTr("Probing SMTP server..."));
        ServerTest.test( serverAddress, "submission" );   //probe port and encryption
    }

    for (i = 0; i < ac_mail.countIdentities(); i++) {
        var j = SetupManager.createIdentity();
        j.setTransport(smtp);
        //templates
        //drafts
        //fcc
        ac_mail.fillIdentitiy(i,j);
        if (i == ac_mail.defaultIdentity()) {
            identity = j;
        }
    }

    if (ac_mail.countIdentities() == 0) {
        identity = SetupManager.createIdentity();
        identity.setEmail(page.widget().emailEdit.text);
        identity.setRealName(page.widget().nameEdit.text);
        identity.setTransport(smtp);
    }

    kolabRes = SetupManager.createResource("akonadi_kolab_resource");
    kolabRes.setEditMode(page2.widget().checkBoxImapEdit.checked);
    kolabRes.setOption("Password", page.widget().passwordEdit.password);
    kolabRes.setOption("UseDefaultIdentity", false);
    kolabRes.setOption("AccountIdentity", identity.uoid());
    kolabRes.setOption("DisconnectedModeEnabled", true);
    kolabRes.setOption("IntervalCheckTime", 60);
    kolabRes.setOption("SubscriptionEnabled", true);
    kolabRes.setOption("SieveSupport", true);

    if (ac_mail_stat) {
        ac_mail.fillImapServer(0, kolabRes);
    } else if (page2.widget().lineEditImap.text) {
        var serverAddress = page2.widget().lineEditImap.text;
        kolabRes.setOption("ImapServer", serverAddress);
        kolabRes.setOption("UserName", page.widget().emailEdit.text.trim());

        if (!servertest_running) {
            servertest_mode = 2;
            servertest_running = true;
            SetupManager.setupInfo(qsTr("Probing IMAP server..."));
            ServerTest.test(serverAddress, "imap");
        }       kolabRes.setOption("Authentication", 7);
    }

    if (ac_ldap_stat) {
        for (i = 0; i < ac_ldap.countLdapServers(); i++) {
            var ldap = SetupManager.createLdap();
            ldap.setEditMode(page2.widget().checkBoxLdapEdit.checked);
            ac_ldap.fillLdapServer(i,ldap);
        }
    } else if (page2.widget().checkBoxLdapEdit.checked) {
        var ldap = SetupManager.createLdap();
        ldap.setEditMode(page2.widget().checkBoxLdapEdit.checked);
        ldap.setPassword(page.widget().passwordEdit.password);
        ldap.setUser(page.widget().emailEdit.text);
        ldap.setServer(guessServerName());
    }

    if (ac_freebusy_stat) {
        var korganizer = SetupManager.createConfigFile("akonadi-calendarrc");
        korganizer.setEditMode(page2.widget().checkBoxFreebusyEdit.checked);
        korganizer.setEditName("freebusy");
        korganizer.setName("korganizer");
        ac_freebusy.fillFreebusyServer(0,korganizer);
    } else if (page2.widget().checkBoxFreebusyEdit.checked) {
        var korganizer = SetupManager.createConfigFile("akonadi-calendarrc");
        korganizer.setEditMode(page2.widget().checkBoxFreebusyEdit.checked);
        korganizer.setEditName("freebusy");
        korganizer.setName( "korganizer" );
        korganizer.setConfig( "FreeBusy Retrieve", "FreeBusyFullDomainRetrieval","true");
        korganizer.setConfig( "FreeBusy Retrieve", "FreeBusyRetrieveAuto", "true" );
        korganizer.setConfig( "FreeBusy Retrieve", "FreeBusyRetrieveUrl", "https://" + guessServerName()  + "/freebusy/" );
    }

    if (!servertest_running) {
        SetupManager.execute();
    }
}

try {
  ServerTest.testFail.connect(testResultFail);
  ServerTest.testResult.connect(testOk);

  page.widget().emailEdit.textChanged.connect(validateInput);
  page.widget().passwordEdit.passwordChanged.connect(validateInput);

  page.pageLeftNext.connect(checkAutoconfig);
  page2.pageLeftNext.connect(setup);
} catch (e) {
  print(e);
}

validateInput();*/
