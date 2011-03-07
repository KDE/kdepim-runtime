/*  This file is part of the KDE project
    Copyright (C) 2011 Kevin Krammer, kevin.krammer@gmx.at

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

#include "imapcacheadapter.h"

#include "kmigratorbase.h"

#include "mixedmaildirsettings.h"
#include "mixedmaildirstore.h"

#include <filestore/collectionmodifyjob.h>

#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agentmanager.h>
#include <akonadi/collection.h>

#include <KLocale>

#include <QQueue>

using namespace Akonadi;

class ImapCacheAdapter::Private
{
  ImapCacheAdapter *const q;

  public:
    Private( ImapCacheAdapter *parent, MixedMaildirStore *store )
      : q( parent ), mStore( store )
    {
    }

    void processNextCollection();

    void createResource();

  public:
    MixedMaildirStore *const mStore;

    QQueue<Collection> mPendingCollections;

  public: // slots
    void createResourceResult( KJob *job );
    void collectionModifyResult( KJob *job );
};

void ImapCacheAdapter::Private::processNextCollection()
{
  kDebug() << mPendingCollections.count() << "pending collections";

  if ( mPendingCollections.isEmpty() ) {
    createResource();
    return;
  }

  const Collection collection = mPendingCollections.dequeue();
  kDebug() << "processing: name=" << collection.name()
           << "remoteId=" << collection.remoteId()
           << "parent=" << collection.parentCollection().remoteId();

  FileStore::CollectionModifyJob *job = mStore->modifyCollection( collection );
  connect( job, SIGNAL( result( KJob* ) ), q, SLOT( collectionModifyResult( KJob* ) ) );
}

void ImapCacheAdapter::Private::createResource()
{
  const QString typeId = QLatin1String( "akonadi_mixedmaildir_resource" );
  AgentInstanceCreateJob *job = new AgentInstanceCreateJob( typeId, q );
  connect( job, SIGNAL( result( KJob* ) ), q, SLOT( createResourceResult( KJob* ) ) );
  job->start();
}

void ImapCacheAdapter::Private::createResourceResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << "Creation of MixedMaildir resource for local cache adapter failed:" << job->errorString();
    emit q->finished( KMigratorBase::Error,
                      i18n( "Could not create adapter for previous KMail version's disconnected IMAP cache" ) );
    return;
  }

  AgentInstanceCreateJob *createJob = qobject_cast<AgentInstanceCreateJob*>( job );

  AgentInstance instance = createJob->instance();

  OrgKdeAkonadiMixedMaildirSettingsInterface *iface = new OrgKdeAkonadiMixedMaildirSettingsInterface(
    "org.freedesktop.Akonadi.Resource." + instance.identifier(),
    "/Settings", QDBusConnection::sessionBus(), q );

  if (!iface->isValid() ) {
    kError() << "Failed to obtain D-Bus interface for remote configuration of local cache adapter resource" << instance.identifier();
    delete iface;

    AgentManager::self()->removeInstance( instance );

    emit q->finished( KMigratorBase::Error,
                      i18n( "Could not configure adapter for previous KMail version's disconnected IMAP cache" ) );
    return;
  }

  const QString name =
    i18nc( "@title account name", "Previous KMail's disconnected IMAP cache" );
  instance.setName( name );
  iface->setPath( mStore->path() );

  // make sure the config is saved
  iface->writeConfig();
  delete iface;

  instance.reconfigure();

  emit q->finished( KMigratorBase::Success, i18n( "Access to previous KMail version's cache enabled" ) );
}

void ImapCacheAdapter::Private::collectionModifyResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << "Rename of DIMAP top level collection failed:" << job->errorString();
  }

  processNextCollection();
}

ImapCacheAdapter::ImapCacheAdapter( MixedMaildirStore *store, QObject *parent )
  : QObject( parent ), d( new Private( this, store ) )
{
}

ImapCacheAdapter::~ImapCacheAdapter()
{
  delete d;
}

void ImapCacheAdapter::addAccount( const QString &topLevelFolder, const QString &accountName )
{
  Collection collection;
  collection.setRemoteId( topLevelFolder );
  collection.setParentCollection( d->mStore->topLevelCollection() );
  collection.setName( accountName );

  d->mPendingCollections << collection;
}

void ImapCacheAdapter::start()
{
  d->processNextCollection();
}

#include "imapcacheadapter.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
