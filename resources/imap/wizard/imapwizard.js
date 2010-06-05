/*
    Copyright (c) 2009 Montel Laurent <montel@kde.org>

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
var page = Dialog.addPage( "imapwizard.ui", "Personal Settings" );

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
  if ( page.widget().incommingAddress.text.trim() == "" ) {
    page.setValid( false );
  } else {
    page.setValid( true );
  }
}

var stage = 1;
var identity;

function setup()
{
  if ( stage == 1 ) {
    identity = SetupManager.createIdentity();
    identity.setEmail( SetupManager.email() );
    identity.setRealName( SetupManager.name() );

    ServerTest.test( page.widget().incommingAddress.text.trim(), "imap" );
  } else {
    ServerTest.test( page.widget().outgoingAddress.text.trim(), "smtp" );
  }
}

function testResultFail()
{
  testOk( -1 );
}

function testOk( arg )
{
  if (stage == 1) {
    var imapRes = SetupManager.createResource( "akonadi_imap_resource" );
    imapRes.setOption( "ImapServer", page.widget().incommingAddress.text.trim() );
    imapRes.setOption( "UserName", page.widget().userName.text );
    imapRes.setOption( "Password", SetupManager.password() );
    imapRes.setOption( "DisconnectedModeEnabled", page.widget().disconnectedMode.checked );
    imapRes.setOption( "UseDefaultIdentity", false );
    imapRes.setOption( "AccountIdentity", identity.uoid() );
    if ( arg == "ssl" ) { 
      // The ENUM used for authentication (in the imap resource only)
      imapRes.setOption( "Safety", "SSL"); // SSL/TLS
      imapRes.setOption( "Authentication", 7 ); // ClearText
      imapRes.setOption( "ImapPort", 993 );
    } else if ( arg == "tls" ) { // tls is really STARTTLS
      imapRes.setOption( "Safety", "STARTTLS");  // STARTTLS
      imapRes.setOption( "Authentication", 7 ); // ClearText
      imapRes.setOption( "ImapPort", 143 );
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
    SetupManager.execute();
  }
}

connect( ServerTest, "testFail()", this, "testResultFail()" );
connect( ServerTest, "testResult(QString)", this, "testOk(QString)" );
connect( page.widget().incommingAddress, "textChanged(QString)", this, "serverChanged(QString)" );
connect( page, "pageLeftNext()", this, "setup()" );

validateInput();
