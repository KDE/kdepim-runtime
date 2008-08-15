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

#include <KDialog>

class QListWidget;

class InfoDialog : public KDialog
{
  Q_OBJECT
  public:
    InfoDialog();
    ~InfoDialog();

  public slots:
    void successMessage( const QString &msg );
    void infoMessage( const QString &msg );
    void errorMessage( const QString &msg );

    void migratorAdded();
    void migratorDone();

  private:
    QListWidget *mList;
    int mMigratorCount;
};

#endif
