/*  This file is part of the KDE project
    Copyright (C) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "directaccessstore.h"

#include <akonadi/collection.h>
#include <akonadi/entitydisplayattribute.h>

#include <QFileInfo>

using namespace Akonadi::FileStore;

class AbstractDirectAccessStore::AbstractDirectAccessStore::Private
{
  public:
    Private( AbstractDirectAccessStore *parent )
      : mParent( parent )
    {
    }

  public:
    QFileInfo mFileInfo;
    Collection mTopLevelCollection;

  private:
    AbstractDirectAccessStore *mParent;
};

AbstractDirectAccessStore::AbstractDirectAccessStore( QObject *parent )
  : QObject( parent ), d( new Private( this ) )
{
}

AbstractDirectAccessStore::~AbstractDirectAccessStore()
{
  delete d;
}

Akonadi::Collection AbstractDirectAccessStore::topLevelCollection() const
{
  return d->mTopLevelCollection;
}

void AbstractDirectAccessStore::setFileName( const QString &fileName )
{
  d->mFileInfo = QFileInfo( fileName );
  d->mFileInfo.makeAbsolute();

  d->mTopLevelCollection.setRemoteId( d->mFileInfo.absoluteFilePath() );
  d->mTopLevelCollection.setName( d->mFileInfo.fileName() );

  Akonadi::EntityDisplayAttribute *attribute =
    d->mTopLevelCollection.attribute<EntityDisplayAttribute>();
  if ( attribute != 0 ) {
    attribute->setDisplayName( d->mFileInfo.fileName() );
  }
}

QString AbstractDirectAccessStore::fileName() const
{
  return d->mFileInfo.absoluteFilePath();
}

void AbstractDirectAccessStore::setTopLevelCollection( const Collection &collection )
{
  d->mTopLevelCollection = collection;
}

#include "directaccessstore.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
