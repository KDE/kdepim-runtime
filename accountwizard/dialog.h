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

#ifndef DIALOG_H
#define DIALOG_H

#include <kassistantdialog.h>

class Page;

class Dialog : public KAssistantDialog
{
  Q_OBJECT
  public:
    explicit Dialog( QWidget *parent = 0 );

    /* reimpl */ void next();
    /* reimpl */ void back();

  public slots:
    Q_SCRIPTABLE QObject* addPage( const QString &uiFile, const QString &title );
  private slots:
    void slotNextPage();
  private:
    KPageWidgetItem* addPage( Page* page, const QString &title );

  private:
    KPageWidgetItem* mLastPage;
};

#endif
