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
    
    title: i18n("Autoconfiguration")
    
    property alias imapSearch: imapLabelField.text
    property alias imapServerName: imapServerField.text
    property bool imapEdit
    
    property alias smtpSearch: smtpLabelField.text
    property alias smtpServerName: smtpServerField.text
    property bool smtpEdit
    
    property string acMailStat;
    property var acMail;
    property var acFreebusy;
    property string acFreebusyStat;
    property var acLdap;
    property string acLdapStat;
    
    property bool showImapSettings: true
    property bool showSmtpSettings: true
    
    property string email;
    property string password;
    
    Component.onCompleted: {
        acMail = SetupManager.ispDB('autoconfigkolabmail');
        acMail.ispdbFinished.connect(function(stat) {
            acMailStat = stat;
            if (stat) {
                showImapSettings = false;
                showSmtpSettings = false;
            } else {
                showImapSettings = true;
                showSmtpSettings = true;
            }
        });
        acMail.info.connect(function(text) {
            imapLabelField.text = text;
            smtpLabelField.text = text;
        });
        acMail.setEmail(email);
        acMail.setPassword(password);
        acMail.start();

        acFreebusy = SetupManager.ispDB('autoconfigkolabfreebusy');
        acFreebusy.ispdbFinished.connect(function(stat) {
            acFreebusyStat = stat;
        }); 
        acFreebusy.info.connect(function(text) {
            freebusyStatus.text = text;
        });
        acFreebusy.setEmail(email);
        acFreebusy.setPassword(password);
        acFreebusy.start();

        acLdap = SetupManager.ispDB('autoconfigkolabldap');
        acLdap.ispdbFinished.connect(function(text) {
            // old code was stting text for checbox to manaul edit
        });
        acLdap.info.connect(function(text) {
            ldapStatus.text = text;
        });
        acLdap.setEmail(email);
        acLdap.setPassword(password);
        acLdap.start();
    }
    
    Kirigami.FormLayout {
        Item {
            visible: showImapSettings
            Kirigami.FormData.label: i18n("IMAP configuration")
            Kirigami.FormData.isSection: true
        }
        QQC2.Label {
            id: imapLabelField
            visible: showImapSettings
            Kirigami.FormData.label: i18n("Status:")
        }
        QQC2.TextField {
            id: imapServerField
            visible: showImapSettings
            Kirigami.FormData.label: i18n("Server Name:")
        }
        QQC2.CheckBox {
            id: checkBoxSmptEdit
            text: i18n("Manual Edit")
        }
        
        Kirigami.Separator {
            Kirigami.FormData.isSection: true
        }
        // SMTP configuration
        Item {
            visible: showSmtpSettings
            Kirigami.FormData.label: i18n("SMTP configuration")
            Kirigami.FormData.isSection: true
        }
        QQC2.Label {
            id: smtpLabelField
            visible: showSmtpSettings
            Kirigami.FormData.label: i18n("Status:")
        }
        QQC2.TextField {
            id: smtpServerField
            visible: showSmtpSettings
            Kirigami.FormData.label: i18n("Server Name:")
        }
        QQC2.CheckBox {
            id: checkBoxSmptEdit
            text: i18n("Manual Edit")
        }
        Kirigami.Separator {
            Kirigami.FormData.isSection: true
        }
        
        // Freebusy config
        Item {
            Kirigami.FormData.label: i18n("Freebusy configuraiton")
            Kirigami.FormData.isSection: true
        }
        QQC2.Label {
            id: freebusyStatus
            visible: text.length > 0
            Kirigami.FormData.label: i18n("Status:")
        }
        QQC2.CheckBox {
            id: freebusy
            text: i18n("Create Freebusy configuration")
        }
        
        Kirigami.Separator {
            Kirigami.FormData.isSection: true
        }
        
        // LDAP config
        Item {
            Kirigami.FormData.label: i18n("LDAP configuraiton")
            Kirigami.FormData.isSection: true
        }
        QQC2.Label {
            id: ldapStatus
            visible: text.length > 0
            Kirigami.FormData.label: i18n("Status:")
        }
        QQC2.CheckBox {
            id: ldap
            text: i18n("Create LDAP configuration")
        }
    }
    
    function setup() {
        SetupManager.openWallet();
        smtp = SetupManager.createTransport("smtp");
        smtp.editMode = checkBoxSmtpEdit.checked;
        smtp.password = password;

        if (acMailStat) {
            acMail.fillSmtpServer(0, smtp);
        } else if (smtpServerName.text) {
            var serverAddress = smtpServerName.text;
            servertestRunning = true;
            servertestMode = 1;
            smtp.name = serverAddress;
            smtp.host = serverAddress);
            smtp.username = email;
            smtp.authenticationType = "plain";

            SetupManager.setupInfo(qsTr("Probing SMTP server..."));
            ServerTest.test(serverAddress, "submission");   //probe port and encryption
        }

        for (i = 0; i < acMail.countIdentities(); i++) {
            var j = SetupManager.createIdentity();
            j.setTransport(smtp);
            //templates
            //drafts
            //fcc
            acMail.fillIdentitiy(i, j);
            if (i == acMail.defaultIdentity()) {
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
        kolabRes.setEditMode(checkBoxImapEdit.checked);
        kolabRes.setOption("Password", password);
        kolabRes.setOption("UseDefaultIdentity", false);
        kolabRes.setOption("AccountIdentity", identity.uoid());
        kolabRes.setOption("DisconnectedModeEnabled", true);
        kolabRes.setOption("IntervalCheckTime", 60);
        kolabRes.setOption("SubscriptionEnabled", true);
        kolabRes.setOption("SieveSupport", true);

        if (acMailStat) {
            acMail.fillImapServer(0, kolabRes);
        } else if (imapServerName.text.length > 0) {
            const serverAddress = imapServerName.text
            kolabRes.setOption("ImapServer", serverAddress);
            kolabRes.setOption("UserName", page.widget().emailEdit.text.trim());

            if (!servertest_running) {
                servertestMode = 2;
                servertestRunning = true;
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
}
