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

#ifndef KRESMIGRATOR_H
#define KRESMIGRATOR_H

#include "kresmigratorbase.h"

#include <kresources/manager.h>
#include <kresources/resource.h>

#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KStandardDirs>

template <typename T> class KResMigrator : public KResMigratorBase
{
  public:
    KResMigrator( const QString &type, const QString &bridgeType ) :
      KResMigratorBase( type, bridgeType ),
      mConfig( 0 ),
      mManager( 0 ),
      mBridgeManager( 0 ),
      mClientBridgeFound( false )
    {
    }

    virtual ~KResMigrator()
    {
      delete mConfig;
      delete mManager;
    }

    void migrate()
    {
      mManager = new KRES::Manager<T>( mType );
      mManager->readConfig();
      const QString kresCfgFile = KStandardDirs::locateLocal( "config", QString( "kresources/%1/stdrc" ).arg( mType ) );
      mConfig = new KConfig( kresCfgFile );
      mIt = mManager->begin();
      migrateNext();
    }

    void migrateNext()
    {
      while ( mIt != mManager->end() ) {
        if ( (*mIt)->type() == "akonadi" ) {
          mClientBridgeFound = true;
          emit successMessage( i18n( "Client-side bridge already set up." ) );
          ++mIt;
          continue;
        }
        KConfigGroup cfg( KGlobal::config(), "Resource " + (*mIt)->identifier() );
        if ( migrationState( *mIt ) == None ) {
          emit infoMessage( i18n( "Trying to migrate '%1'...", (*mIt)->resourceName() ) );
          mPendingBridgedResources.removeAll( (*mIt)->identifier() );
          T* res = *mIt;
          mCurrentKResource = res;
          ++mIt;
          bool nativeAvailable = mBridgeOnly ? false : migrateResource( res );
          if ( !nativeAvailable ) {
            emit infoMessage( i18n( "No native backend for '%1' available.", res->resourceName() ) );
            migrateToBridge( res, mBridgeType );
          }
          return;
        }
        if ( migrationState( *mIt ) == Bridged && !mPendingBridgedResources.contains( (*mIt)->identifier() ) )
          mPendingBridgedResources << (*mIt)->identifier();
        if ( migrationState( *mIt ) == Complete )
          emit successMessage( i18n( "'%1' has already been migrated.", (*mIt)->resourceName() ) );
        ++mIt;
      }
      if ( mIt == mManager->end() ) {
        delete mConfig;
        mConfig = 0;
        migrateBridged();
      }
    }

    void migrateBridged()
    {
      delete mBridgeManager;
      mBridgeManager = 0;

      if ( mPendingBridgedResources.isEmpty() ) {
        setupClientBridge();
        return;
      }
      const QString resId = mPendingBridgedResources.takeFirst();
      KConfigGroup resMigrationCfg( KGlobal::config(), "Resource " + resId );
      const QString akoResId = resMigrationCfg.readEntry( "ResourceIdentifier", "" );
      if ( akoResId.isEmpty() ) {
        emit errorMessage( "No Akonadi agent identifier specified for previously bridged resource '" + resId + "'" );
        migrateNext();
        return;
      }

      const QString bridgedCfgFile = KStandardDirs::locateLocal( "config", QString( "%1rc" ).arg( akoResId ) );
      kDebug() << bridgedCfgFile;
      mConfig = new KConfig( bridgedCfgFile );

      KRES::Manager<T> *mBridgeManager = new KRES::Manager<T>( mType );
      mBridgeManager->readConfig( mConfig );
      if ( !mBridgeManager->standardResource() ) {
        emit errorMessage( "Bridged resource '" + resId + "' has no standard resource." );
        migrateNext();
        return;
      }

      T *res = mBridgeManager->standardResource();
      emit infoMessage( i18n( "Trying to migrate '%1' from compatibility bridge to native backend...", res->resourceName() ) );
      mCurrentKResource = res;
      bool nativeAvailable = migrateResource( res );
      if ( !nativeAvailable ) {
        emit successMessage( i18n( "No native backend avaiable, keeping compatibility bridge for '%1'", res->resourceName() ) );
        migrateNext();
      }
    }

    virtual bool migrateResource( T *res ) = 0;

    KConfigGroup kresConfig( KRES::Resource* res ) const
    {
      return KConfigGroup( mConfig, "Resource_" + res->identifier() );
    }

    T* currentResource()
    {
      T* res = dynamic_cast<T*>( mCurrentKResource );
      Q_ASSERT( res );
      return res;
    }

  private:
    void setupClientBridge()
    {
      if ( !mClientBridgeFound ) {
        emit infoMessage( i18n( "Setting up client-side bridge..." ) );
        T* clientBridge = mManager->createResource( "akonadi" );
        if ( clientBridge ) {
          clientBridge->setResourceName( i18n("Akonadi Compatibility Resource") );
          mManager->add( clientBridge );
          mManager->setStandardResource( clientBridge );
          emit successMessage( i18n( "Client-side bridge set up successfully." ) );
        } else {
          emit errorMessage( i18n( "Could not create client-side bridge, check if Akonadi KResource bridge is installed." ) );
        }
      }
      deleteLater();
    }

  private:
    KConfig *mConfig;
    KRES::Manager<T> *mManager, *mBridgeManager;
    typedef typename KRES::Manager<T>::Iterator ResourceIterator;
    ResourceIterator mIt;
    bool mClientBridgeFound;
};

#endif
