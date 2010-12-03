/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                         a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#include "settingspasswordrequester.h"

#include <QtCore/QTimer>

#include <KDE/KMessageBox>
#include <KDE/KLocale>

#include <mailtransport/transportbase.h>

#include "imapresource.h"
#include "settings.h"

SettingsPasswordRequester::SettingsPasswordRequester( ImapResource *resource, QObject *parent )
  : PasswordRequesterInterface( parent ), m_resource( resource )
{

}

void SettingsPasswordRequester::requestPassword( RequestType request, const QString &serverError )
{
  if ( request == WrongPasswordRequest ) {
    QMetaObject::invokeMethod( this, "askUserInput", Qt::QueuedConnection, Q_ARG(QString, serverError) );
  } else {
    connect( Settings::self(), SIGNAL(passwordRequestCompleted(QString, bool)),
             this, SLOT(onPasswordRequestCompleted(QString, bool)) );
    Settings::self()->requestPassword();
  }
}

void SettingsPasswordRequester::askUserInput( const QString &serverError )
{
  // the credentials were not ok....
  int i = KMessageBox::questionYesNoCancelWId( m_resource->winIdForDialogs(),
                                               i18n( "The server refused the supplied username and password. "
                                                     "Do you want to go to the settings, have another attempt "
                                                     "at logging in, or do nothing?\n\n"
                                                     "%1", serverError ),
                                               i18n( "Could Not Authenticate" ),
                                               KGuiItem( i18n( "Settings" ) ),
                                               KGuiItem( i18nc( "Input username/password manually and not store them", "Single Input" ) ) );

  if ( i == KMessageBox::Yes ) {
    int result = m_resource->configureDialog( m_resource->winIdForDialogs() );
    if ( result==QDialog::Accepted ) {
      emit done( ReconnectNeeded );
    } else {
      emit done( UserRejected );
    }
  } else if ( i == KMessageBox::No ) {
    connect( Settings::self(), SIGNAL(passwordRequestCompleted(QString, bool)),
             this, SLOT(onPasswordRequestCompleted(QString, bool)) );
    Settings::self()->requestManualAuth();
  } else {
    emit done( UserRejected );
  }
}

void SettingsPasswordRequester::onPasswordRequestCompleted( const QString &password, bool userRejected )
{
  disconnect( Settings::self(), SIGNAL(passwordRequestCompleted(QString, bool)),
              this, SLOT(onPasswordRequestCompleted(QString, bool)) );

  if ( userRejected ) {
    emit done( UserRejected );
  } else if ( password.isEmpty() && (Settings::self()->authentication() != MailTransport::Transport::EnumAuthenticationType::GSSAPI) ) {
    emit done( EmptyPasswordEntered );
  } else {
    emit done( PasswordRetrieved, password );
  }
}

#include "settingspasswordrequester.moc"

