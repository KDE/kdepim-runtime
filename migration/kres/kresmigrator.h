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

#include <akonadi/agentmanager.h>
#include <kresources/manager.h>
#include <kresources/resource.h>

#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KStandardDirs>

using namespace Akonadi;

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
      delete mBridgeManager;
      mBridgeManager = 0;
      delete mManager;
      mManager = 0;
      delete mConfig;
      mConfig = 0;
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
          mClientBridgeIdentifier = (*mIt)->identifier();
          mClientBridgeFound = true;
          emit message( Skip, i18n( "Client-side bridge already set up." ) );
          ++mIt;
          continue;
        }
        KConfigGroup cfg( KGlobal::config(), "Resource " + (*mIt)->identifier() );
        if ( migrationState( (*mIt)->identifier() ) == None ) {
          emit message( Info, i18n( "Trying to migrate '%1'...", (*mIt)->resourceName() ) );
          mPendingBridgedResources.removeAll( (*mIt)->identifier() );
          T* res = *mIt;
          mCurrentKResource = res;
          ++mIt;

          if ( res->type() == "kolab" )
          {
            // check if kolab resource exists. If not, create one.
            Akonadi::AgentInstance kolabAgent = Akonadi::AgentManager::self()->instance( "akonadi_kolab_resource" );
            if ( !kolabAgent.isValid() )
              emit message( Info, i18n( "Attempting to create kolab resource" ) );
              createAgentInstance( "akonadi_kolab_resource", this, SLOT(kolabResourceCreated()) );
          } else {
            bool nativeAvailable = mBridgeOnly ? false : migrateResource( res );
            if ( !nativeAvailable ) {
              emit message( Info, i18n( "No native backend for '%1' available.", res->resourceName() ) );
              migrateToBridge( res, mBridgeType );
            }
          }
          return;
        }
        if ( migrationState( (*mIt)->identifier() ) == Bridged &&
             !mPendingBridgedResources.contains( (*mIt)->identifier() ) )
          mPendingBridgedResources << (*mIt)->identifier();
        if ( migrationState( (*mIt)->identifier() ) == Complete )
          emit message( Skip, i18n( "'%1' has already been migrated.", (*mIt)->resourceName() ) );
        ++mIt;
      }
      if ( !mBridgeType.isEmpty() ) {

        if ( mIt == mManager->end() ) {
          migrateBridged();
        }
      } else {
        deleteLater();
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
        emit message( Error, i18n("No Akonadi agent identifier specified for previously bridged resource '%1'", resId ));
        migrateNext();
        return;
      }

      const QString bridgedCfgFile = KStandardDirs::locateLocal( "config", QString( "%1rc" ).arg( akoResId ) );
      kDebug() << bridgedCfgFile;
      mConfig = new KConfig( bridgedCfgFile );

      mBridgeManager = new KRES::Manager<T>( mType );
      mBridgeManager->readConfig( mConfig );
      if ( !mBridgeManager->standardResource() ) {
        emit message( Error, i18n("Bridged resource '%1' has no standard resource.", resId) );
        migrateNext();
        return;
      }

      T *res = mBridgeManager->standardResource();
      emit message( Info, i18n( "Trying to migrate '%1' from compatibility bridge to native backend...", res->resourceName() ) );
      mCurrentKResource = res;
      bool nativeAvailable = migrateResource( res );
      if ( !nativeAvailable ) {
        emit message( Skip, i18n( "No native backend available, keeping compatibility bridge for '%1'.", res->resourceName() ) );
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

    void migrationCompleted( const Akonadi::AgentInstance &instance )
    {
      // do an intial sync so the resource shows up in the folder tree at least
      AgentInstance nonConstInstance = instance;
      nonConstInstance.synchronize();

      // check if this one was previously bridged and remove the bridge
      KConfigGroup cfg( KGlobal::config(), "Resource " + mCurrentKResource->identifier() );
      const QString bridgeId = cfg.readEntry( "ResourceIdentifier", "" );
      if ( bridgeId != instance.identifier() ) {
        const AgentInstance bridge = AgentManager::self()->instance( bridgeId );
        AgentManager::self()->removeInstance( bridge );
      }

      setMigrationState( mCurrentKResource->identifier(), Complete, instance.identifier(), mType );
      emit message( Success, i18n( "Migration of '%1' succeeded.", mCurrentKResource->resourceName() ) );
      migrationCompletedHelper( instance );
    }

    void migratedToBridge(const Akonadi::AgentInstance & instance)
    {
      mBridgingInProgress = false;
      setMigrationState( mCurrentKResource->identifier(), Bridged, instance.identifier(), mType );
      emit message( Success, i18n( "Migration of '%1' to compatibility bridge succeeded.", mCurrentKResource->resourceName() ) );
      migrationCompletedHelper( instance );
    }

  private:

    // This is called when a native migration or a migration to a bridged resource is
    // successfully completed.
    void migrationCompletedHelper( const Akonadi::AgentInstance & instance )
    {
      if ( mManager->standardResource() == mCurrentKResource ) {
        mAgentForOldDefaultResource = instance.identifier();
      }
      mManager->setActive( mCurrentKResource, false );
      mCurrentKResource = 0;
      migrateNext();
    }

    void setupClientBridge()
    {
      if ( !mOmitClientBridge ) {
        if ( !mClientBridgeFound ) {
          emit message( Info, i18n( "Setting up client-side bridge..." ) );
          T* clientBridge = mManager->createResource( "akonadi" );
          if ( clientBridge ) {
            mClientBridgeIdentifier = clientBridge->identifier();
            clientBridge->setResourceName( i18n("Akonadi Compatibility Resource") );
            mManager->add( clientBridge );
            mManager->setStandardResource( clientBridge );
            emit message( Info, i18n( "Client-side bridge set up successfully." ) );
          } else {
            emit message( Error, i18n( "Could not create client-side bridge, check if Akonadi KResource bridge is installed." ) );
          }
        }

        mManager->writeConfig();
        const QString keyName( "DefaultAkonadiResourceIdentifier" );
        KConfigGroup clientBridgeConfig( mConfig, "Resource_" + mClientBridgeIdentifier );
        if ( !clientBridgeConfig.hasKey( keyName ) &&
            !mAgentForOldDefaultResource.isEmpty() ) {
          clientBridgeConfig.writeEntry( keyName, mAgentForOldDefaultResource );
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
    QString mClientBridgeIdentifier;
    QString mAgentForOldDefaultResource;
};

#endif
