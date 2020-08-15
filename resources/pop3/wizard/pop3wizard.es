/*
    SPDX-FileCopyrightText: 2009 Montel Laurent <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// add this function to trim user input of whitespace when needed
String.prototype.trim = function() { return this.replace(/^\s+|\s+$/g, ""); };

var page = Dialog.addPage( "pop3wizard.ui", qsTr("Personal Settings") );

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

function setup()
{
  var pop3Res = SetupManager.createResource( "akonadi_pop3_resource" );
  pop3Res.setOption( "Host", page.widget().incommingAddress.text.trim() );
  pop3Res.setOption( "Login", page.widget().userName.text.trim() );
  pop3Res.setOption( "Password", SetupManager.password() );

  var smtp = SetupManager.createTransport( "smtp" );
  smtp.setName( SetupManager.name() );
  smtp.setHost( page.widget().outgoingAddress.text.trim() );
  smtp.setEncryption( "SSL" );

  SetupManager.execute();
}

page.widget().incommingAddress.textChanged.connect( serverChanged );
page.pageLeftNext.connect( setup );
validateInput();
