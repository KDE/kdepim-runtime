/*
    Copyright (c) 2008 Bertjan Broeksema <b.broeksema@kdemail.net>
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

#include "singlefileresourcebase.h"

#include <akonadi/changerecorder.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemfetchscope.h>

#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <KDebug>
#include <KDirWatch>
#include <KLocale>
#include <KStandardDirs>

#include <QtCore/QDir>

using namespace Akonadi;

SingleFileResourceBase::SingleFileResourceBase( const QString & id )
  : ResourceBase( id ), mDownloadJob( 0 ), mUploadJob( 0 )
{
  connect( &mDirtyTimer, SIGNAL( timeout() ), SLOT( writeFile() ) );
  mDirtyTimer.setSingleShot( true );

  connect( this, SIGNAL( reloadConfiguration() ), SLOT( reloadFile() ) );
  QTimer::singleShot( 0, this, SLOT( readFile() ) );

  changeRecorder()->itemFetchScope().fetchFullPayload();
  changeRecorder()->fetchCollection( true );

  connect( KDirWatch::self(), SIGNAL( dirty( QString ) ), SLOT( fileChanged( QString ) ) );
}

QString SingleFileResourceBase::cacheFile() const
{
  return KStandardDirs::locateLocal( "cache", "akonadi/" + identifier() );
}

void SingleFileResourceBase::setSupportedMimetypes( const QStringList & mimeTypes, const QString &icon )
{
  mSupportedMimetypes = mimeTypes;
  mCollectionIcon = icon;
}

void SingleFileResourceBase::collectionChanged( const Akonadi::Collection & collection )
{
  QString newName = collection.name();
  if ( collection.hasAttribute<EntityDisplayAttribute>() ) {
    EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>();
    if ( !attr->displayName().isEmpty() )
      newName = attr->displayName();
  }

  if ( newName != name() )
    setName( newName );
}

void SingleFileResourceBase::reloadFile()
{
  // Update the network setting.
  setNeedsNetwork( !mCurrentUrl.isEmpty() && !mCurrentUrl.isLocalFile() );

  // if we have something loaded already, make sure we write that back in case
  // the settings changed
  if ( !mCurrentUrl.isEmpty() )
    writeFile();

  readFile();
  synchronize();
}

void SingleFileResourceBase::handleProgress( KJob *, unsigned long pct )
{
  emit percent( pct );
}

void SingleFileResourceBase::fileChanged( const QString & fileName )
{
  if ( fileName != mCurrentUrl.path() )
    return;

  // handle conflicts
  if ( mDirtyTimer.isActive() && !mCurrentUrl.isEmpty() ) {
    const KUrl prevUrl = mCurrentUrl;
    int i = 0;
    QString lostFoundFileName;
    do {
      lostFoundFileName = KStandardDirs::locateLocal( "data", identifier() + QDir::separator()
          + prevUrl.fileName() + "-" + QString::number( ++i ) );
    } while ( KStandardDirs::exists( lostFoundFileName ) );
    mCurrentUrl = KUrl( lostFoundFileName );
    writeFile();
    emit warning( i18n( "The file '%1' was changed on disk while there were still pending changes in Akonadi. "
        "To avoid dataloss, a backup of the internal changes has been created at '%2'.",
         prevUrl.prettyUrl(), mCurrentUrl.prettyUrl() ) );
    mCurrentUrl = prevUrl;
  }

  readFile();
  synchronize();
}

void SingleFileResourceBase::slotDownloadJobResult( KJob *job )
{
  if ( job->error() && job->error() != KIO::ERR_DOES_NOT_EXIST ) {
    emit status( Broken, i18n( "Could not load file '%1'.", mCurrentUrl.prettyUrl() ) );
  } else {
    readFromFile( KUrl( cacheFile() ).url() );
  }

  mDownloadJob = 0;
  KGlobal::deref();

  emit status( Idle, i18nc( "@info:status", "Ready" ) );
}

void SingleFileResourceBase::slotUploadJobResult( KJob *job )
{
  if ( job->error() ) {
    emit status( Broken, i18n( "Could not save file '%1'.", mCurrentUrl.prettyUrl() ) );
  }

  mUploadJob = 0;
  KGlobal::deref();

  emit status( Idle, i18nc( "@info:status", "Ready" ) );
}

#include "singlefileresourcebase.moc"
