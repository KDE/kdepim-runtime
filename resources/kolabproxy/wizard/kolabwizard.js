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

// TODO: i18n??
var page = Dialog.addPage( "kolabwizard.ui", "Personal Settings" );

var userChangedServerAddress = false;

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
  if ( page.widget().serverAddress.text == "" ) {
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
  if ( stage == 1 ) {
    SetupManager.createResource( "akonadi_kolabproxy_resource" );

    identity = SetupManager.createIdentity();
    identity.setEmail( SetupManager.email() );
    identity.setRealName( SetupManager.name() );
    
    ServerTest.test( page.widget().serverAddress.text, "imap" );
  } else { // stage 2
    var smtp = SetupManager.createTransport( "smtp" );
    smtp.setName( page.widget().serverAddress.text );
    smtp.setHost( page.widget().serverAddress.text );
    smtp.setPort( 465 );
    smtp.setEncryption( "SSL" );
    smtp.setUsername( SetupManager.email() );
    smtp.setPassword( SetupManager.password() );

    var ldap = SetupManager.createLdap();
    ldap.setUser( SetupManager.email() );
    ldap.setServer( page.widget().serverAddress.text );

    var korganizer = SetupManager.createConfigFile( "korganizerrc" );
    korganizer.setName( "korganizer" );
    korganizer.setConfig( "FreeBusy Retrieve", "FreeBusyFullDomainRetrieval","true");
    korganizer.setConfig( "FreeBusy Retrieve", "FreeBusyRetrieveAuto", "true" );
    korganizer.setConfig( "FreeBusy Retrieve", "FreeBusyRetrieveUrl", "https://" + page.widget().serverAddress.text  + "/freebusy/" );
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
    imapRes.setOption( "ImapServer", page.widget().serverAddress.text );
    imapRes.setOption( "UserName", SetupManager.email() );
    imapRes.setOption( "Password", SetupManager.password() );
    imapRes.setOption( "UseDefaultIdentity", false );
    imapRes.setOption( "AccountIdentity", identity.uoid() );
    imapRes.setOption( "SubscriptionEnabled", true );
    if ( arg == "ssl" ) { 
      // The ENUM used for authentication (in the imap resource only)
      // is KIMAP::LoginJob::AuthenticationMode
      imapRes.setOption( "Safety", "SSL" ); // SSL/TLS
      imapRes.setOption( "Authentication", 0 ); // ClearText
      imapRes.setOption( "ImapPort", 993 );
    } else if ( arg == "tls" ) { // tls is really STARTTLS
      imapRes.setOption( "Safety", "STARTTLS" );  // STARTTLS
      imapRes.setOption( "Authentication", 0 ); // ClearText
      imapRes.setOption( "ImapPort", 143 );
    } else {
      imapRes.setOption( "Safety", "NONE" );  // No encryption
      imapRes.setOption( "Authentication", 0 ); // ClearText
      imapRes.setOption( "ImapPort", 143 );
    }
    stage = 2;
    setup();
}

connect( ServerTest, "testFail()", this, "testResultFail()" );
connect( ServerTest, "testResult(QString)", this, "testOk(QString)" );
connect( page.widget().serverAddress, "textChanged(QString)", this, "serverChanged(QString)" );
connect( page, "pageLeftNext()", this, "setup()" );

validateInput();
