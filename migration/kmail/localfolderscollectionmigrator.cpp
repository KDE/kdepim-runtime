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

#include "localfolderscollectionmigrator.h"

#include <akonadi/kmime/specialmailcollections.h>
#include <akonadi/collection.h>

#include <KConfig>
#include <KConfigGroup>
#include <KLocale>

#include <QHash>

using namespace Akonadi;

typedef QHash<QString, SpecialMailCollections::Type> TypeHash;

class LocalFoldersCollectionMigrator::Private
{
  LocalFoldersCollectionMigrator *const q;

  public:
    explicit Private( LocalFoldersCollectionMigrator *parent )
      : q( parent )
    {
    }

  public:
    TypeHash mSystemFolders;
};

LocalFoldersCollectionMigrator::LocalFoldersCollectionMigrator( const AgentInstance &resource, MixedMaildirStore *store, QObject *parent )
  : AbstractCollectionMigrator( resource, store, parent ), d( new Private( this ) )
{
}

LocalFoldersCollectionMigrator::~LocalFoldersCollectionMigrator()
{
  delete d;
}

void LocalFoldersCollectionMigrator::setKMailConfig( const KSharedConfigPtr &config )
{
  AbstractCollectionMigrator::setKMailConfig( config );

  const KConfigGroup group( config, QLatin1String( "General" ) );

  QString name = group.readEntry( QLatin1String( "inboxFolder" ), i18nc( "mail folder name for role inbox",  "inbox" ) );
  d->mSystemFolders.insert( name, SpecialMailCollections::Inbox );

  name = group.readEntry( QLatin1String( "outboxFolder" ), i18nc( "mail folder name for role outbox",  "outbox" ) );
  d->mSystemFolders.insert( name, SpecialMailCollections::Outbox );

  name = group.readEntry( QLatin1String( "sentFolder" ), i18nc( "mail folder name for role sent-mail",  "sent-mail" ) );
  d->mSystemFolders.insert( name, SpecialMailCollections::SentMail );

  name = group.readEntry( QLatin1String( "trashFolder" ), i18nc( "mail folder name for role trash",  "trash" ) );
  d->mSystemFolders.insert( name, SpecialMailCollections::Trash );

  name = group.readEntry( QLatin1String( "draftsFolder" ), i18nc( "mail folder name for role drafts",  "drafts" ) );
  d->mSystemFolders.insert( name, SpecialMailCollections::Drafts );

  name = group.readEntry( QLatin1String( "templatesFolder" ), i18nc( "mail folder name for role templates",  "templates" ) );
  d->mSystemFolders.insert( name, SpecialMailCollections::Templates );
}

void LocalFoldersCollectionMigrator::migrateCollection( const Collection &collection, const QString &folderId )
{
  Q_UNUSED( folderId );

  emit status( collection.name() );

  if ( collection.parentCollection() == Collection::root() ) {
    registerAsSpecialCollection( SpecialMailCollections::Root );
  } else {
    const TypeHash::const_iterator typeIt = d->mSystemFolders.constFind( collection.name() );
    if ( typeIt != d->mSystemFolders.constEnd() ) {
      registerAsSpecialCollection( *typeIt );
    }
  }

  collectionProcessed();
}

#include "localfolderscollectionmigrator.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
