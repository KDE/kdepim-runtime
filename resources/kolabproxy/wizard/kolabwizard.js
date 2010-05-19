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

function setup()
{
  SetupManager.createResource( "akonadi_kolabproxy_resource" );

  var identity = SetupManager.createIdentity();
  identity.setEmail( SetupManager.email() );
  identity.setRealName( SetupManager.name() );

  var imapRes = SetupManager.createResource( "akonadi_imap_resource" );
  imapRes.setOption( "ImapServer", page.widget().serverAddress.text );
  imapRes.setOption( "UserName", SetupManager.email() );
  imapRes.setOption( "Password", SetupManager.password() );
  imapRes.setOption( "UseDefaultIdentity", false );
  imapRes.setOption( "AccountIdentity", identity.uoid() );
  imapRes.setOption( "SubscriptionEnabled", true );

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

connect( page.widget().serverAddress, "textChanged(QString)", this, "serverChanged(QString)" );
connect( page, "pageLeftNext()", this, "setup()" );

validateInput();
