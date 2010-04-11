/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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
function emailChanged( arg )
{
  validateInput();
  if ( userChangedServerAddress == true ) {
    return;
  }

  var pos = arg.indexOf( "@" );
  if ( pos >= 0 && (pos + 1) < arg.length ) {
    var server = arg.slice( pos + 1, arg.length );
    page.kolabWizard.serverAddress.setText( server );
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
  if ( page.kolabWizard.serverAddress.text == "" || page.kolabWizard.emailAddress.text == "" ) {
    page.setValid( false );
  } else {
    page.setValid( true );
  }
}

function setup()
{
  SetupManager.createResource( "akonadi_kolabproxy_resource" );

  var identity = SetupManager.createIdentity();
  identity.setEmail( page.kolabWizard.emailAddress.text );
  identity.setRealName( page.kolabWizard.fullName.text );

  var imapRes = SetupManager.createResource( "akonadi_imap_resource" );
  imapRes.setOption( "ImapServer", page.kolabWizard.serverAddress.text );
  imapRes.setOption( "UserName", page.kolabWizard.emailAddress.text );
  imapRes.setOption( "Password", page.kolabWizard.password.text );
  imapRes.setOption( "UseDefaultIdentity", false );
  imapRes.setOption( "AccountIdentity", identity.uoid() );
  imapRes.setOption( "SubscriptionEnabled", true );

  var smtp = SetupManager.createTransport( "smtp" );
  smtp.setName( page.kolabWizard.serverAddress.text );
  smtp.setHost( page.kolabWizard.serverAddress.text );
  smtp.setPort( 465 );
  smtp.setEncryption( "SSL" );
  smtp.setUsername( page.kolabWizard.emailAddress.text );
  smtp.setPassword( page.kolabWizard.password.text );

  var ldap = SetupManager.createLdap();
  ldap.setUser( page.kolabWizard.emailAddress.text );
  ldap.setServer( page.kolabWizard.serverAddress.text );

  var korganizer = SetupManager.createConfigFile( "korganizerrc" );
  korganizer.setName( "korganizer" );
  korganizer.setConfig( "FreeBusy Retrieve", "FreeBusyFullDomainRetrieval","true");
  korganizer.setConfig( "FreeBusy Retrieve", "FreeBusyRetrieveAuto", "true" );
  korganizer.setConfig( "FreeBusy Retrieve", "FreeBusyRetrieveUrl", "https://" + page.kolabWizard.serverAddress.text  + "/freebusy/" );
  SetupManager.execute();
}

connect( page.kolabWizard.emailAddress, "textChanged(QString)", this, "emailChanged(QString)" );
connect( page.kolabWizard.serverAddress, "textChanged(QString)", this, "serverChanged(QString)" );
connect( page, "pageLeftNext()", this, "setup()" );
validateInput();
