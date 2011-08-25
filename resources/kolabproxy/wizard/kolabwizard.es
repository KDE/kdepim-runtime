/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>

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

// TODO: i18n??
var page = Dialog.addPage( "kolabwizard.ui", "Personal Settings" );
var userChangedServerAddress = false;

page.widget().nameEdit.text = SetupManager.name()
page.widget().emailEdit.text = SetupManager.email()
page.widget().passwordEdit.text = SetupManager.password()
guessServerName();

if ( SetupManager.personalDataAvailable() ) {
  page.widget().nameEdit.visible = false;
  page.widget().nameLabel.visible = false;
  page.widget().emailEdit.visible = false;
  page.widget().emailLabel.visible = false;
  page.widget().passwordEdit.visible = false;
  page.widget().passwordLabel.visible = false;
}


function guessServerName()
{
  if ( userChangedServerAddress == true ) {
    return;
  }

  var email = page.widget().emailEdit.text;
  var pos = email.indexOf( "@" );
  if ( pos >= 0 && (pos + 1) < email.length ) {
    var server = email.slice( pos + 1, email.length );
    page.widget().serverAddress.text = server;
  }

  userChangedServerAddress = false;
}

function emailChanged( arg )
{
  validateInput();
  guessServerName();
}

function serverChanged( arg )
{
  validateInput();
  if ( arg == "" ) {
    userChangedServerAddress = false;
  } else {
    userChangedServerAddress = true;
  }
}

function validateInput()
{
  if ( page.widget().serverAddress.text.trim() == "" || page.widget().emailEdit.text.trim() == "" ) {
    page.setValid( false );
  } else {
    page.setValid( true );
  }
}

// stage 1, setup identity and run imap server test
// stage 2, smtp setup (no server test), ldap and korganizer
var stage = 1;
var identity; // global so it can be accesed in setup and testOk

function setup()
{
  var serverAddress = page.widget().serverAddress.text.trim();
  if ( stage == 1 ) {
    SetupManager.openWallet();
    SetupManager.createResource( "akonadi_kolabproxy_resource" );

    identity = SetupManager.createIdentity();
    identity.setEmail( page.widget().emailEdit.text );
    identity.setRealName( page.widget().nameEdit.text );

    ServerTest.test( serverAddress, "imap" );
  } else { // stage 2
    var smtp = SetupManager.createTransport( "smtp" );
    smtp.setName( serverAddress );
    smtp.setHost( serverAddress );
    smtp.setPort( 465 );
    smtp.setEncryption( "SSL" );
    smtp.setAuthenticationType( "plain" ); // using plain is ok, because we are using SSL.
    smtp.setUsername( page.widget().emailEdit.text );
    smtp.setPassword( page.widget().passwordEdit.text );
    identity.setTransport( smtp );

    var ldap = SetupManager.createLdap();
    ldap.setUser( page.widget().emailEdit.text );
    ldap.setServer( serverAddress );

    var korganizer = SetupManager.createConfigFile( "korganizerrc" );
    korganizer.setName( "korganizer" );
    korganizer.setConfig( "FreeBusy Retrieve", "FreeBusyFullDomainRetrieval","true");
    korganizer.setConfig( "FreeBusy Retrieve", "FreeBusyRetrieveAuto", "true" );
    korganizer.setConfig( "FreeBusy Retrieve", "FreeBusyRetrieveUrl", "https://" + serverAddress  + "/freebusy/" );
    SetupManager.execute();
  }
}

function testResultFail()
{
  testOk( -1 );
}

function testOk( arg )
{
    print("testOk arg =", arg);
    var imapRes = SetupManager.createResource( "akonadi_imap_resource" );
    imapRes.setName( page.widget().serverAddress.text.trim() );
    imapRes.setOption( "ImapServer", page.widget().serverAddress.text.trim() );
    imapRes.setOption( "UserName", page.widget().emailEdit.text.trim() );
    imapRes.setOption( "Password", page.widget().passwordEdit.text.trim() );
    imapRes.setOption( "UseDefaultIdentity", false );
    imapRes.setOption( "AccountIdentity", identity.uoid() );
    imapRes.setOption( "DisconnectedModeEnabled", true );
    imapRes.setOption( "IntervalCheckTime", 60 );
    imapRes.setOption( "SubscriptionEnabled", true );
    imapRes.setOption( "SieveSupport", true );
    imapRes.setOption( "Authentication", 7 ); // ClearText
    if ( arg == "ssl" ) { 
      // The ENUM used for authentication (in the imap resource only)
      imapRes.setOption( "Safety", "SSL" ); // SSL/TLS
      imapRes.setOption( "ImapPort", 993 );
    } else if ( arg == "tls" ) { // tls is really STARTTLS
      imapRes.setOption( "Safety", "STARTTLS" );  // STARTTLS
      imapRes.setOption( "ImapPort", 143 );
    } else if ( arg == "none" ) {
      imapRes.setOption( "Safety", "NONE" );  // No encryption
      imapRes.setOption( "ImapPort", 143 );
    } else {
      // safe default fallback in case server test failed
      imapRes.setOption( "Safety", "STARTTLS" );
      imapRes.setOption( "ImapPort", 143 );
    }
    stage = 2;
    setup();
}

try {
  ServerTest.testFail.connect(testResultFail);
  ServerTest.testResult.connect(testOk);
  page.widget().emailEdit.textChanged.connect(emailChanged);
  page.widget().serverAddress.textChanged.connect(serverChanged);
  page.pageLeftNext.connect(setup);
} catch (e) {
  print(e);
}

validateInput();
