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
