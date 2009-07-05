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

#ifndef AKONADI_SINGLEFILERESOURCE_H
#define AKONADI_SINGLEFILERESOURCE_H

#include "singlefileresourcebase.h"

#include <akonadi/entitydisplayattribute.h>

#include <kio/job.h>
#include <KDirWatch>
#include <KLocale>
#include <KStandardDirs>

#include <QFile>
#include <QDir>

namespace Akonadi
{

/**
 * Base class for single file based resources.
 */
template <typename Settings>
class SingleFileResource : public SingleFileResourceBase
{
  public:
    SingleFileResource( const QString &id ) : SingleFileResourceBase( id )
    {
      // The resource needs network when the path refers to a non local file.
      setNeedsNetwork( !KUrl( Settings::self()->path() ).isLocalFile() );
    }

    /**
     * Indicate that there are pending changes.
     */
    void fileDirty()
    {
      if( !mDirtyTimer.isActive() ) {
        mDirtyTimer.setInterval( Settings::self()->autosaveInterval() * 60 * 1000 );
        mDirtyTimer.start();
      }
    }

    /**
     * Read changes from the backend file.
     */
    void readFile()
    {
      if ( KDirWatch::self()->contains( mCurrentUrl.toLocalFile() ) )
        KDirWatch::self()->removeFile( mCurrentUrl.toLocalFile() );

      const bool nameWasChanged = mCurrentUrl.fileName() != name() && !mCurrentUrl.isEmpty();

      if ( Settings::self()->path().isEmpty() ) {
        emit status( Broken, i18n( "No file selected." ) );
        return;
      }

      mCurrentUrl = KUrl( Settings::self()->path() );

      if ( mCurrentUrl.isLocalFile() )
      {
        if ( !nameWasChanged )
          setName( mCurrentUrl.fileName() );

        // check if the file does not exist yet, if so, create it
        if ( !QFile::exists( mCurrentUrl.toLocalFile() ) ) {
          QFile f( mCurrentUrl.toLocalFile() );

          // first create try to create the directory the file should be located in
          QDir dir = QFileInfo(f).dir();
          if ( ! dir.exists() ) {
            dir.mkpath( dir.path() );
          }

          if ( f.open( QIODevice::WriteOnly ) && f.resize( 0 ) ) {
            emit status( Idle, i18nc( "@info:status", "Ready" ) );
          } else {
            emit status( Broken, i18n( "Could not create file '%1'.", mCurrentUrl.prettyUrl() ) );
            mCurrentUrl.clear();
            return;
          }
        }

        if ( !readFromFile( mCurrentUrl.toLocalFile() ) ) {
          mCurrentUrl = KUrl(); // reset so we don't accidentally overwrite the file
          return;
        }

        if ( Settings::self()->monitorFile() )
          KDirWatch::self()->addFile( mCurrentUrl.toLocalFile() );

        emit status( Idle, i18nc( "@info:status", "Ready" ) );
        synchronize();
      }
      else
      {
        if ( mDownloadJob )
        {
          emit error( i18n( "Another download is still in progress." ) );
          return;
        }

        if ( mUploadJob )
        {
          emit error( i18n( "Another file upload is still in progress." ) );
          return;
        }

        KGlobal::ref();

        // NOTE: Test what happens with remotefile -> save, close before save is finished.
        mDownloadJob = KIO::file_copy( mCurrentUrl, KUrl( cacheFile() ), -1, KIO::Overwrite | KIO::DefaultFlags | KIO::HideProgressInfo );
        connect( mDownloadJob, SIGNAL( result( KJob * ) ),
                SLOT( slotDownloadJobResult( KJob * ) ) );
        connect( mDownloadJob, SIGNAL( percent( KJob *, unsigned long ) ),
                 SLOT( handleProgress( KJob *, unsigned long ) ) );

        emit status( Running, i18n( "Downloading remote file." ) );
      }
    }

    /**
     * Write changes to the backend file.
     */
    void writeFile()
    {
      if ( Settings::self()->readOnly() ) {
        emit error( i18n( "Trying to write to a read-only file: '%1'.", Settings::self()->path() ) );
        return;
      }

      mDirtyTimer.stop();

      // We don't use the Settings::self()->path() here as that might have changed
      // and in that case it would probably cause data lose.
      if ( mCurrentUrl.isEmpty() ) {
        emit status( Broken, i18n( "No file specified." ) );
        return;
      }

      if ( mCurrentUrl.isLocalFile() ) {
        KDirWatch::self()->stopScan();
        const bool writeResult = writeToFile( mCurrentUrl.toLocalFile() );
        KDirWatch::self()->startScan();
        if ( !writeResult )
          return;

        emit status( Idle, i18nc( "@info:status", "Ready" ) );
      } else {
        // Check if there is a download or an upload in progress.
        if ( mDownloadJob ) {
          emit error( i18n( "A download is still in progress." ) );
          return;
        }

        if ( mUploadJob ) {
          emit error( i18n( "Another file upload is still in progress." ) );
          return;
        }

        // Write te items to the localy cached file.
        if ( !writeToFile( cacheFile() ) )
          return;

        KGlobal::ref();
        // Start a job to upload the localy cached file to the remote location.
        mUploadJob = KIO::file_copy( KUrl( cacheFile() ), mCurrentUrl, -1, KIO::Overwrite | KIO::DefaultFlags | KIO::HideProgressInfo );
        connect( mUploadJob, SIGNAL( result( KJob * ) ),
                SLOT( slotUploadJobResult( KJob * ) ) );
        connect( mUploadJob, SIGNAL( percent( KJob *, unsigned long ) ),
                 SLOT( handleProgress( KJob *, unsigned long ) ) );

        emit status( Running, i18n( "Uploading cached file to remote location." ) );
      }
    }

  protected:
    void retrieveCollections()
    {
      Collection c;
      c.setParent( Collection::root() );
      c.setRemoteId( Settings::self()->path() );
      c.setName( identifier() );
      QStringList mimeTypes;
      c.setContentMimeTypes( mSupportedMimetypes );
      if ( Settings::self()->readOnly() ) {
        c.setRights( Collection::CanChangeCollection );
      } else {
        Collection::Rights rights;
        rights |= Collection::CanChangeItem;
        rights |= Collection::CanCreateItem;
        rights |= Collection::CanDeleteItem;
        rights |= Collection::CanChangeCollection;
        c.setRights( rights );
      }
      EntityDisplayAttribute* attr = c.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
      attr->setDisplayName( name() );
      attr->setIconName( mCollectionIcon );
      Collection::List list;
      list << c;
      collectionsRetrieved( list );
    }
};

}

#endif
