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
var page = Dialog.addPage( "imapwizard.ui", "Personal Settings" );

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
    page.imapWizard.incommingAddress.setText( server );
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
  if ( page.imapWizard.incommingAddress.text == "" || page.imapWizard.emailAddress.text == "" ) {
    page.setValid( false );
  } else {
    page.setValid( true );
  }
}

function setup()
{
  var imapRes = SetupManager.createResource( "akonadi_imap_resource" );
  imapRes.setOption( "ImapServer", page.imapWizard.incommingAddress.text );
  imapRes.setOption( "UserName", page.imapWizard.emailAddress.text );
  imapRes.setOption( "Password", page.imapWizard.password.text );
  imapRes.setOption( "DisconnectedModeEnabled", "true" );

  var smtp = SetupManager.createTransport( "smtp" );
  smtp.setName( page.imapWizard.outgoingAddress.text );
  smtp.setHost( page.imapWizard.outgoingAddress.text );
  smtp.setPort( 143 );
  smtp.setEncryption( "NONE" );

  SetupManager.execute();
}

connect( page.imapWizard.emailAddress, "textChanged(QString)", this, "emailChanged(QString)" );
connect( page.imapWizard.serverAddress, "textChanged(QString)", this, "serverChanged(QString)" );
connect( page, "pageLeftNext()", this, "setup()" );
validateInput();
