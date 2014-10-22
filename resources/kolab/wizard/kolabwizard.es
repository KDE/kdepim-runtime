/*
    Copyright (c) 2014 Sandro Knauß <knauss@kolabsys.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

// add this function to trim user input of whitespace when needed
String.prototype.trim = function() { return this.replace(/^\s+|\s+$/g, ""); };

var page = Dialog.addPage("kolabwizard.ui", qsTr("Personal Settings"));
var page2 = Dialog.addPage("kolabwizard2.ui", qsTr("Autoconfiguration"));
var userChangedServerAddress = false;

page.widget().nameEdit.text = SetupManager.name()
page.widget().emailEdit.text = SetupManager.email()
page.widget().passwordEdit.text = SetupManager.password()

function guessServerName()
{
    if (userChangedServerAddress == true) {
        return;
    }
    var email = page.widget().emailEdit.text;
    var pos = email.indexOf("@");
    if (pos >= 0 && (pos + 1) < email.length) {
      var server = email.slice(pos + 1, email.length);
      return server;
    }
}

function emailChanged(arg)
{
  validateInput();
}

function validateInput()
{
  if (page.widget().emailEdit.text.trim() == "") {
    page.setValid(false);
  } else {
    page.setValid(true);
  }
}

var identity; // global so it can be accesed in setup and testOk

var kolabRes;
var smtp;
var imapRes;

var ac_mail;
var ac_freebusy;
var ac_ldap;

function checkAutoconfig()
{
    ac_mail = SetupManager.ispDB('autoconfigkolabmail');
    ac_mail.ispdbFinished.connect(mail_finished);
    ac_mail.info.connect(mail_text);
    ac_mail.setEmail(page.widget().emailEdit.text);
    ac_mail.setPassword(page.widget().passwordEdit.text);
    ac_mail.start();

    ac_freebusy = SetupManager.ispDB('autoconfigkolabfreebusy');
    ac_freebusy.ispdbFinished.connect(freebusy_finished);
    ac_freebusy.info.connect(freebusy_text);
    ac_freebusy.setEmail(page.widget().emailEdit.text);
    ac_freebusy.setPassword(page.widget().passwordEdit.text);
    ac_freebusy.start();

    ac_ldap = SetupManager.ispDB('autoconfigkolabldap');
    ac_ldap.ispdbFinished.connect(ldap_finished);
    ac_ldap.info.connect(ldap_text);
    ac_ldap.setEmail(page.widget().emailEdit.text);
    ac_ldap.setPassword(page.widget().passwordEdit.text);
    ac_ldap.start();
}

guessServerName();

function mail_finished(stat) {
    if (stat) {
        page2.widget().checkBoxImap.checked = true;
        page2.widget().checkBoxImap.enabled = true;
        page2.widget().checkBoxSmtp.checked = true;
        page2.widget().checkBoxSmtp.enabled = true;
    } else {
        page2.widget().checkBoxImap.checked = false;
        page2.widget().checkBoxImap.enabled = false;
        page2.widget().checkBoxSmtp.checked = false;
        page2.widget().checkBoxSmtp.enabled = false;
    }
}

function mail_text(text) {
    page2.widget().labelImapSearch.text = text;
    page2.widget().labelSmtpSearch.text = text;
}

function freebusy_finished(stat) {
    if (stat) {
        page2.widget().checkBoxFreebusy.checked = true;
        page2.widget().checkBoxFreebusy.enabled = true;
    } else {
        page2.widget().checkBoxFreebusy.checked = false;
        page2.widget().checkBoxFreebusy.enabled = false;
    }
}

function freebusy_text(text) {
    page2.widget().labelFreebusySearch.text = text;
}


function ldap_finished(stat) {
    if (stat) {
        page2.widget().checkBoxLdap.checked = true;
        page2.widget().checkBoxLdap.enabled = true;
    } else {
        page2.widget().checkBoxLdap.checked = false;
        page2.widget().checkBoxLdap.enabled = false;
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
    smtp.setPassword(page.widget().passwordEdit.text);

    if (page2.widget().checkBoxSmtp.checked) {
        ac_mail.fillSmtpServer(0, smtp);
    } else if (guessServerName()) {
        var serverAddress = guessServerName();
        smtp.setName(serverAddress);
        smtp.setHost(serverAddress);
        smtp.setPort(465);
        smtp.setEncryption("SSL");
        smtp.setAuthenticationType("plain"); // using plain is ok, because we are using SSL.
        smtp.setUsername(page.widget().emailEdit.text);
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
    kolabRes.setOption("Password", page.widget().passwordEdit.text);
    kolabRes.setOption("UseDefaultIdentity", false);
    kolabRes.setOption("AccountIdentity", identity.uoid());
    kolabRes.setOption("DisconnectedModeEnabled", true);
    kolabRes.setOption("IntervalCheckTime", 60);
    kolabRes.setOption("SubscriptionEnabled", true);
    kolabRes.setOption("SieveSupport", true);

    if (page2.widget().checkBoxImap.checked) {
        ac_mail.fillImapServer(0, kolabRes);
    } else if (guessServerName()) {
        var serverAddress = guessServerName();
        kolabRes.setOption("ImapServer", serverAddress);
        kolabRes.setOption("UserName", page.widget().emailEdit.text.trim());
        kolabRes.setOption("Authentication", 7);
        kolabRes.setOption("Safety", "STARTTLS");
        kolabRes.setOption("ImapPort", 143);
    }

    if (page2.widget().checkBoxLdap.checked) {
        for (i = 0; i < ac_ldap.countLdapServers(); i++) {
            var ldap = SetupManager.createLdap();
            ldap.setEditMode(page2.widget().checkBoxLdapEdit.checked);
            ac_ldap.fillLdapServer(i,ldap);
        }
    } else if (page2.widget().checkBoxLdapEdit.checked) {
        var ldap = SetupManager.createLdap();
        ldap.setEditMode(page2.widget().checkBoxLdapEdit.checked);
        ldap.setPassword(page.widget().passwordEdit.text);
        ldap.setUser(page.widget().nameEdit.text);
        ldap.setServer(guessServerName());
    }

    if (page2.widget().checkBoxFreebusy.checked) {
        var korganizer = SetupManager.createConfigFile("akonadi-calendarrc");
        korganizer.setEditMode(page2.widget().checkBoxFreebusyEdit.checked);
        korganizer.setEditName("freebusy");
        korganizer.setName("korganizer");
        ispdb.fillFreebusyServer(0,korganizer);
    } else if (page2.widget().checkBoxFreebusyEdit.checked) {
        var korganizer = SetupManager.createConfigFile("akonadi-calendarrc");
        korganizer.setEditMode(page2.widget().checkBoxFreebusyEdit.checked);
        korganizer.setEditName("freebusy");
        korganizer.setName( "korganizer" );
        korganizer.setConfig( "FreeBusy Retrieve", "FreeBusyFullDomainRetrieval","true");
        korganizer.setConfig( "FreeBusy Retrieve", "FreeBusyRetrieveAuto", "true" );
        korganizer.setConfig( "FreeBusy Retrieve", "FreeBusyRetrieveUrl", "https://" + guessServerName()  + "/freebusy/" );
    }

    SetupManager.execute();
}

try {
  page.widget().emailEdit.textChanged.connect(emailChanged);
  page.pageLeftNext.connect(checkAutoconfig);
  page2.pageLeftNext.connect(setup);
} catch (e) {
  print(e);
}

validateInput();
