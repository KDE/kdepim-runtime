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

// TODO: i18n??
var page = Dialog.addPage( "pop3wizard.ui", "Personal Settings" );

var userChangedServerAddress = false;
function emailChanged( arg )
{
  validateInput();
  if ( userChangedServerAddress == true ) {
    return;
  }

  var pos = arg.indexOf( "@" );
  if ( pos >= 0 && (pos + 1) < arg.length ) {
    var server = arg.slice( pos + 1, arg.length );
    page.pop3Wizard.incommingAddress.setText( server );
  }

  userChangedServerAddress = false;
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
  if ( page.pop3Wizard.incommingAddress.text == "" || page.pop3Wizard.emailAddress.text == "" ) {
    page.setValid( false );
  } else {
    page.setValid( true );
  }
}

function setup()
{
  var pop3Res = SetupManager.createResource( "akonadi_pop3_resource" );
  pop3Res.setOption( "Host", page.pop3Wizard.incommingAddress.text );
  pop3Res.setOption( "Login", page.pop3Wizard.emailAddress.text );
  //pop3Res.setOption( "Password", page.pop3Wizard.password.text );

  var smtp = SetupManager.createTransport( "smtp" );
  smtp.setName( page.pop3Wizard.outgoingAddress.text );
  smtp.setHost( page.pop3Wizard.outgoingAddress.text );
  smtp.setEncryption( "NONE" );

  SetupManager.execute();
}

connect( page.pop3Wizard.emailAddress, "textChanged(QString)", this, "emailChanged(QString)" );
connect( page.pop3Wizard.incommingAddress, "textChanged(QString)", this, "serverChanged(QString)" );
connect( page, "pageLeftNext()", this, "setup()" );
validateInput();
