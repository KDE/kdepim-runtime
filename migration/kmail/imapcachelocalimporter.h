/*  This file is part of the KDE project
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef IMAPCACHELOCALIMPORTER_H
#define IMAPCACHELOCALIMPORTER_H

#include <QObject>

namespace Akonadi
{
  class Item;
}

class MixedMaildirStore;

class ImapCacheLocalImporter : public QObject
{
  Q_OBJECT

  public:
    explicit ImapCacheLocalImporter( MixedMaildirStore *store, QObject *parent = 0 );

    ~ImapCacheLocalImporter();

    void setTopLevelFolder( const QString &topLevelFolder );

    void setAccountName( const QString &accountName );

  Q_SIGNALS:
    void importFinished( const QString &error );

  public Q_SLOTS:
    void startImport();

  private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void createResourceResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void configureResource() )
    Q_PRIVATE_SLOT( d, void collectionFetchResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void collectionCreateResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void itemFetchResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void itemCreateResult( KJob* ) )
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
