/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef INFODIALOG_H
#define INFODIALOG_H

#include "kmigratorbase.h"

#include <KDialog>

class QListWidget;

class InfoDialog : public KDialog
{
  Q_OBJECT
  public:
    InfoDialog( bool closeWhenDone = true );
    ~InfoDialog();

  public slots:
    void message( KMigratorBase::MessageType type, const QString &msg );

    void migratorAdded();
    void migratorDone();

    bool hasError() const { return mError; }
    bool hasChange() const { return mChange; }

  private:
    QListWidget *mList;
    int mMigratorCount;
    bool mError;
    bool mChange;
    bool mCloseWhenDone;
};

#endif
