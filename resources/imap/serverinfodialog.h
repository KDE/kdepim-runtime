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

#ifndef SERVERINFODIALOG_H
#define SERVERINFODIALOG_H

#include <KDialog>

class ImapResource;
namespace Ui {
class ServerInfo;
}

class ServerInfoDialog : public KDialog
{
    Q_OBJECT
public:
    explicit ServerInfoDialog(ImapResource *parentResource, QWidget *parent);
    ~ServerInfoDialog();
private:
    Ui::ServerInfo *mServerInfoWidget;
};

#endif // SERVERINFODIALOG_H
