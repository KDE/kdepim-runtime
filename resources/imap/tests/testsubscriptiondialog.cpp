/*
   Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
   Author: Kevin Ottens <kevin@kdab.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or ( at your option ) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtGui/QApplication>

#include "imapaccount.h"
#include "subscriptiondialog.h"

int main( int argc, char **argv )
{
  QApplication app(argc, argv);

  if (app.arguments().size() < 5) {
    qWarning("Not enough parameters, expecting: <server> <port> <user> <password>");
    return 1;
  }

  QString server = app.arguments().at(1);
  int port = app.arguments().at(2).toInt();
  QString user = app.arguments().at(3);
  QString password = app.arguments().at(4);

  qDebug() << "Querying:" << server << port << user << password;
  qDebug();

  ImapAccount account;
  account.setServer(server);
  account.setPort(port);
  account.setUserName(user);

  SubscriptionDialog *dialog = new SubscriptionDialog();
  dialog->connectAccount( account, password );

  dialog->show();

  int retcode = app.exec();

  qDebug() << "Subscription changed?" << dialog->isSubscriptionChanged();

  return retcode;
}
