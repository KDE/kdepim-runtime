/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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

#include "serverinfodialog.h"
#include "imapresource.h"
#include "ui_serverinfo.h"

#include <KLocale>

ServerInfoDialog::ServerInfoDialog(ImapResource *parentResource, QWidget *parent)
    : KDialog(parent)
{
    setCaption(
      i18nc( "@title:window Dialog title for dialog showing information about a server",
             "Server Info" ) );
    setButtons( KDialog::Close );
    setAttribute( Qt::WA_DeleteOnClose );

    mServerInfoWidget = new Ui::ServerInfo();
    mServerInfoWidget->setupUi( this );
    setMainWidget( mServerInfoWidget->serverInfo );
    mServerInfoWidget->serverInfo->setPlainText(
      parentResource->serverCapabilities().join( QLatin1String( "\n" ) ) );
}

ServerInfoDialog::~ServerInfoDialog()
{
    delete mServerInfoWidget;
}

