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

// try to guess some defaults
var emailAddr = SetupManager.email();
var pos = emailAddr.indexOf( "@" );
if ( pos >= 0 && (pos + 1) < emailAddr.length ) {
  var server = emailAddr.slice( pos + 1, emailAddr.length );
  page.widget().incommingAddress.text = server;
  page.widget().outgoingAddress.text = server;
  // We must not strip the server from the user identifier.
  // Otherwise the 'user' will be kdabtest1 instead of kdabtest1@demo.kolab.org
  // which fails,
  //var user = emailAddr.slice( 0, pos );
  page.widget().userName.text = emailAddr;
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
    SetupManager.openWallet();
    var imapRes = SetupManager.createResource( "akonadi_imap_resource" );
    imapRes.setOption( "ImapServer", page.widget().incommingAddress.text.trim() );
    imapRes.setOption( "UserName", page.widget().userName.text );
    imapRes.setOption( "Password", SetupManager.password() );
    imapRes.setOption( "DisconnectedModeEnabled", page.widget().disconnectedMode.checked );
    imapRes.setOption( "UseDefaultIdentity", false );
    imapRes.setOption( "AccountIdentity", identity.uoid() );
    imapRes.setOption( "Authentication", 7 ); // ClearText
    if ( arg == "ssl" ) { 
      // The ENUM used for authentication (in the imap resource only)
      imapRes.setOption( "Safety", "SSL"); // SSL/TLS
      imapRes.setOption( "ImapPort", 993 );
    } else if ( arg == "tls" ) { // tls is really STARTTLS
      imapRes.setOption( "Safety", "STARTTLS");  // STARTTLS
      imapRes.setOption( "ImapPort", 143 );
    } else if ( arg == "none" ) {
      imapRes.setOption( "Safety", "NONE" );  // No encryption
      imapRes.setOption( "ImapPort", 143 );
    } else {
      // safe default fallback when servertest failed
      imapRes.setOption( "Safety", "STARTTLS");
      imapRes.setOption( "ImapPort", 143 );
    }
    imapRes.setOption( "IntervalCheckTime", 60 );
    imapRes.setOption( "SubscriptionEnabled", true );

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
    identity.setTransport( smtp );
    SetupManager.execute();
  }
}

try {
  ServerTest.testFail.connect( testResultFail );
  ServerTest.testResult.connect( testOk );
  page.widget().incommingAddress.textChanged.connect( validateInput );
  page.pageLeftNext.connect( setup );
} catch ( e ) {
  print( e );
}

validateInput();
