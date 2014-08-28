/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

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

#include <KLocalizedString>
#include <QDialogButtonBox>
#include <KConfigGroup>
#include <QPushButton>
#include <QVBoxLayout>

ServerInfoDialog::ServerInfoDialog(ImapResourceBase *parentResource, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(
      i18nc( "@title:window Dialog title for dialog showing information about a server",
             "Server Info" ) );
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ServerInfoDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ServerInfoDialog::reject);
    setAttribute( Qt::WA_DeleteOnClose );

    mServerInfoWidget = new Ui::ServerInfo();
    mServerInfoWidget->setupUi( this );
    mainLayout->addWidget(mServerInfoWidget->serverInfo);
    mainLayout->addWidget(buttonBox);
    mServerInfoWidget->serverInfo->setPlainText(
      parentResource->serverCapabilities().join( QLatin1String( "\n" ) ) );
}

ServerInfoDialog::~ServerInfoDialog()
{
    delete mServerInfoWidget;
}

