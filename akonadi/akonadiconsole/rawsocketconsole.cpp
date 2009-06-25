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

#include "rawsocketconsole.h"

#include <akonadi/private/xdgbasedirs_p.h>

#include <KDebug>
#include <KGlobalSettings>
#include <KIcon>

#include <QFile>
#include <QLocalSocket>
#include <QSettings>

using namespace Akonadi;

RawSocketConsole::RawSocketConsole(QWidget* parent) :
  QWidget( parent ),
  mSocket( new QLocalSocket( this ) )
{
  ui.setupUi( this );
  ui.execButton->setIcon( KIcon( "application-x-executable" ) );
  connect( ui.execButton, SIGNAL(clicked()), SLOT(execClicked()) );
  connect( ui.commandEdit, SIGNAL(returnPressed()), SLOT(execClicked()) );
  connect( ui.connectButton, SIGNAL(clicked()), SLOT(connectClicked()) );
  ui.protocolView->setFont( KGlobalSettings::fixedFont() );

  connect( mSocket, SIGNAL(readyRead()), SLOT(dataReceived()) );
  connect( mSocket, SIGNAL(connected()), SLOT(connected()) );
  connect( mSocket, SIGNAL(disconnected()), SLOT(disconnected()) );

  disconnected();
  connectClicked();
}

void RawSocketConsole::execClicked()
{
  const QString command = ui.commandEdit->text().trimmed() + '\n';
  if ( command.isEmpty() )
    return;
  mSocket->write( command.toUtf8() );
  ui.commandEdit->clear();
  ui.protocolView->append( "<font color=\"red\">" + command + "</font>" );
}

void RawSocketConsole::dataReceived()
{
  while ( mSocket->canReadLine() ) {
    const QString line = QString::fromUtf8( mSocket->readLine() );
    ui.protocolView->append( "<font color=\"blue\">" + line + "</font>" );
  }
}

void RawSocketConsole::connected()
{
  ui.connectButton->setChecked( true );
  ui.connectButton->setText( i18n( "Disconnect" ) );
  ui.execButton->setEnabled( true );
  ui.commandEdit->setEnabled( true );
}

void RawSocketConsole::disconnected()
{
  ui.connectButton->setChecked( false );
  ui.connectButton->setText( i18n( "Connect" ) );
  ui.execButton->setEnabled( false );
  ui.commandEdit->setEnabled( false );
}

void RawSocketConsole::connectClicked()
{
  if ( mSocket->state() == QLocalSocket::ConnectedState ) {
    mSocket->close();
  } else {
    const QString connectionConfigFile = XdgBaseDirs::akonadiConnectionConfigFile();
    if ( !QFile::exists( connectionConfigFile ) ) {
      kWarning( 5250 ) << "Akonadi Client Session: connection config file '"
      << "akonadi/akonadiconnectionrc can not be found in '"
      << XdgBaseDirs::homePath( "config" ) << "' nor in any of "
      << XdgBaseDirs::systemPathList( "config" );
    }
    QSettings conSettings( connectionConfigFile, QSettings::IniFormat );
#ifdef Q_OS_WIN  //krazy:exclude=cpp
    const QString namedPipe = conSettings.value( QLatin1String( "Data/NamedPipe" ), QLatin1String( "Akonadi" ) ).toString();
    mSocket->connectToServer( namedPipe );
#else
    const QString defaultSocketDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi" ) );
    const QString path = conSettings.value( QLatin1String( "Data/UnixPath" ), defaultSocketDir + QLatin1String( "/akonadiserver.socket" ) ).toString();
    mSocket->connectToServer( path );
#endif
  }
}

#include "rawsocketconsole.moc"
