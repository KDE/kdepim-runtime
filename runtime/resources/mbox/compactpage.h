/*
    Copyright (c) 2009 Bertjan Broeksema <broeksema@kde.org>

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

#ifndef COMPACTPAGE_H
#define COMPACTPAGE_H

#include <QtGui/QWidget>

#include "ui_compactpage.h"

class KJob;

class CompactPage : public QWidget
{
  Q_OBJECT

  public:
    explicit CompactPage( const QString &collectionId, QWidget *parent = 0 );

  private slots:
    void compact();
    void onCollectionFetchCheck( KJob* );
    void onCollectionFetchCompact( KJob* );
    void onCollectionModify( KJob* );

  private: // Methods
    void checkCollectionId();

  private: // Members
    QString mCollectionId;
    Ui::CompactPage ui;
};

#endif
